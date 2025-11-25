/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Signal
*/

#ifndef SIGNAL_HPP_
#define SIGNAL_HPP_

#include <functional>
#include <vector>

template<typename ...Args>
class Signal {
    public:
        using Slot = std::function<void(Args...)>;

        void connect(const Slot& slot) {
            slots_.push_back(slot);
        }

        void emit(Args... args) {
            for (const auto& slot : slots_) {
                slot(args...);
            }
        }

    private:
        std::vector<Slot> slots_;

};

#endif /* !SIGNAL_HPP_ */

