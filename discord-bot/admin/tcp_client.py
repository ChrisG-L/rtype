"""
TCP Client for R-Type Admin Server.

Connects to TCPAdminServer (port 4127) and executes CLI commands remotely.
Protocol: JSON-RPC over TCP with newline-delimited messages.

Request: {"cmd": "command", "args": "optional args", "token": "auth_token"}\n
Response: {"success": true/false, "output": ["line1", ...], "error": "if any"}\n
"""

import asyncio
import json
import logging
from dataclasses import dataclass
from typing import Optional

logger = logging.getLogger(__name__)


@dataclass
class CommandResult:
    """Result from executing a command on the server."""
    success: bool
    output: list[str]
    error: str = ""


class TCPAdminClient:
    """Async TCP client for R-Type Admin Server with keepalive."""

    KEEPALIVE_INTERVAL = 60  # Send keepalive every 60 seconds

    def __init__(self, host: str, port: int, token: str):
        """
        Initialize the TCP Admin Client.

        Args:
            host: Server hostname or IP
            port: TCP Admin Server port (default 4127)
            token: Admin authentication token
        """
        self.host = host
        self.port = port
        self.token = token
        self._reader: Optional[asyncio.StreamReader] = None
        self._writer: Optional[asyncio.StreamWriter] = None
        self._lock = asyncio.Lock()
        self._connected = False
        self._keepalive_task: Optional[asyncio.Task] = None

    async def connect(self) -> bool:
        """
        Connect to the TCP Admin Server.

        Returns:
            True if connected successfully, False otherwise.
        """
        try:
            self._reader, self._writer = await asyncio.wait_for(
                asyncio.open_connection(self.host, self.port),
                timeout=10.0
            )
            self._connected = True
            logger.info(f"Connected to R-Type Admin Server at {self.host}:{self.port}")
            # Start keepalive task
            self._start_keepalive()
            return True
        except asyncio.TimeoutError:
            logger.error(f"Connection timeout to {self.host}:{self.port}")
            return False
        except ConnectionRefusedError:
            logger.error(f"Connection refused by {self.host}:{self.port}")
            return False
        except Exception as e:
            logger.error(f"Failed to connect to {self.host}:{self.port}: {e}")
            return False

    def _start_keepalive(self) -> None:
        """Start the keepalive background task."""
        if self._keepalive_task is not None:
            self._keepalive_task.cancel()
        self._keepalive_task = asyncio.create_task(self._keepalive_loop())

    async def _keepalive_loop(self) -> None:
        """Send periodic keepalive pings to keep the connection alive."""
        while self._connected:
            try:
                await asyncio.sleep(self.KEEPALIVE_INTERVAL)
                if self._connected and self._writer:
                    # Send a lightweight "help" command as keepalive
                    async with self._lock:
                        if not self._connected or not self._writer:
                            break
                        request = {"cmd": "help", "args": "", "token": self.token}
                        request_json = json.dumps(request) + "\n"
                        self._writer.write(request_json.encode("utf-8"))
                        await self._writer.drain()
                        # Read and discard response
                        response = await asyncio.wait_for(
                            self._reader.readline(),
                            timeout=10.0
                        )
                        if not response:
                            logger.warning("Keepalive: server closed connection")
                            self._connected = False
                            break
                        logger.debug("Keepalive ping successful")
            except asyncio.CancelledError:
                raise  # Re-raise to properly cancel the task
            except Exception as e:
                logger.warning(f"Keepalive failed: {e}")
                self._connected = False
                break

    async def disconnect(self) -> None:
        """Disconnect from the server."""
        # Stop keepalive task first
        if self._keepalive_task is not None:
            self._keepalive_task.cancel()
            try:
                await self._keepalive_task
            except asyncio.CancelledError:
                pass
            self._keepalive_task = None

        if self._writer:
            try:
                self._writer.close()
                await self._writer.wait_closed()
            except Exception:
                pass
        self._connected = False
        self._reader = None
        self._writer = None
        logger.info("Disconnected from R-Type Admin Server")

    async def is_connected(self) -> bool:
        """Check if still connected to server."""
        return self._connected and self._writer is not None

    async def _ensure_connected(self) -> bool:
        """Ensure we have a connection, reconnect if needed."""
        if not await self.is_connected():
            return await self.connect()
        return True

    async def execute(self, cmd: str, args: str = "") -> CommandResult:
        """
        Execute a command on the R-Type server.

        Args:
            cmd: Command name (e.g., "status", "users", "kick")
            args: Optional command arguments

        Returns:
            CommandResult with success status, output lines, and error message.
        """
        async with self._lock:
            if not await self._ensure_connected():
                return CommandResult(
                    success=False,
                    output=[],
                    error="Not connected to server"
                )

            try:
                # Build request
                request = {
                    "cmd": cmd,
                    "args": args,
                    "token": self.token
                }
                request_json = json.dumps(request) + "\n"

                # Send request
                self._writer.write(request_json.encode("utf-8"))
                await self._writer.drain()

                # Read response (until newline)
                response_data = await asyncio.wait_for(
                    self._reader.readline(),
                    timeout=30.0  # 30s timeout for command execution
                )

                if not response_data:
                    self._connected = False
                    return CommandResult(
                        success=False,
                        output=[],
                        error="Server closed connection"
                    )

                # Parse response
                response = json.loads(response_data.decode("utf-8").strip())
                return CommandResult(
                    success=response.get("success", False),
                    output=response.get("output", []),
                    error=response.get("error", "")
                )

            except asyncio.TimeoutError:
                logger.error("Command execution timeout")
                return CommandResult(
                    success=False,
                    output=[],
                    error="Command execution timeout"
                )
            except json.JSONDecodeError as e:
                logger.error(f"Invalid JSON response: {e}")
                return CommandResult(
                    success=False,
                    output=[],
                    error=f"Invalid server response: {e}"
                )
            except Exception as e:
                logger.error(f"Command execution error: {e}")
                self._connected = False
                return CommandResult(
                    success=False,
                    output=[],
                    error=str(e)
                )

    # Convenience methods for common commands

    async def status(self) -> CommandResult:
        """Get server status."""
        return await self.execute("status")

    async def sessions(self) -> CommandResult:
        """List active sessions."""
        return await self.execute("sessions")

    async def users(self) -> CommandResult:
        """List registered users."""
        return await self.execute("users")

    async def user(self, email: str) -> CommandResult:
        """Get user details."""
        return await self.execute("user", email)

    async def kick(self, email: str) -> CommandResult:
        """Kick a user."""
        return await self.execute("kick", email)

    async def ban(self, email: str, reason: str = "") -> CommandResult:
        """Ban a user."""
        args = email
        if reason:
            args = f"{email} {reason}"
        return await self.execute("ban", args)

    async def unban(self, email: str) -> CommandResult:
        """Unban a user."""
        return await self.execute("unban", email)

    async def bans(self) -> CommandResult:
        """List banned users."""
        return await self.execute("bans")

    async def rooms(self) -> CommandResult:
        """List active rooms."""
        return await self.execute("rooms")

    async def room(self, room_code: str) -> CommandResult:
        """Get room details."""
        return await self.execute("room", room_code)

    async def help(self) -> CommandResult:
        """Get list of available commands."""
        return await self.execute("help")

    async def __aenter__(self):
        """Context manager entry."""
        await self.connect()
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        await self.disconnect()
