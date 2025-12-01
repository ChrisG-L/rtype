# Entity Component System documentation
Made by Guillaume LECOCQ

**Note:** *This documentation is partially outdated, for up-to-date documentation please refer to the in-code comments and docs*

## Table of Contents
- [Structure](#structure)
- [The ECS](#the-ecs)
- [Components](#components)
- [Entities](#entities)
- [Systems](#systems)

## Structure

This ECS revolves around a virtual representation of entities.
The main class is the ECS class and is to be manipulated by the user.
Other classes such as Component pools and Systems interface are not to be manipulated.

Here is a list of types, structures and classes that the user can freely manipulate:
```cpp
// User usable types and structs
std::uint16_t EntityID; // This is how entities are represented
std::uint16_t SystemID; // Systems are represented the same way

// Concept defining Systems classes, they must derive from ISystem and have a default constructor (T())
template<typename T>
concept SystemClass = std::is_base_of_v<ISystem, T> && std::is_constructible_v<T>;

// Concept defining a component, components can be anything as long as they can be constructed (typedefs, struct, classes...)
template<typename T>
concept ComponentType = std::is_default_constructible_v<T>;

enum EntityGroup; // Entities can be grouped together, allowing easier access to a large quantity of entities

class ECS; // This is the core class of the ECS, this is what the user is going to use
class ISystem; // System Interface, used to create systems
```
Beside that, there are also abstracted types and classes that the ECS uses in the backgroud, here's a brief summary of what they are:
```cpp
// Abstracted types and structs (the user should not directly use any of these)
struct Entity; // This is how entities are represented within the ECS
struct SystemData; // This is how systems are stored within the ECS

class IComponentPool; // ComponentPool interface

template<Component T>
class ComponentPool : public IComponentPool; // Base for any component storage

```

## The ECS

**TABLE IS OUTDATED !**
*For detailed method implementation and documentation please refer to the in-code documentation*

The ECS class has the following methods:

Parameters in parenthesis are optional
| Template type | return type | function | parameters | Exceptions |description |
|---------------|-------------|----------|------------|------------|-------------|
|               |std::size_t                |currentEntityCount();              |                           |                                                   |Returns the current active entity count                                                        |
|               |bool                       |entityIsActive();                  |EntityID                   |                                                   |Returns if the entity is active                                                                |
|               |ECS::EntityID              |entityCreate();                    |                           |                                                   |Creates an entity and returns its id                                                           |
|               |void                       |entitySetGroup();                  |EntityID, EntityGroup      |                                                   |Sets the group for the entity                                                                  |
|               |void                       |entityDelete(EntityID);            |EntityID                   |                                                   |Deletes the entity                                                                             |
|               |std::vector<EntityID>      |getEntityGroup(EntityGroup);       |EntityGroup                |                                                   |Returns a list of entity attached to a group                                                   |
|Components...  |std::vector<EntityID>      |getEntitiesByComponentsAllOf();    |                           |                                                   |Returns a list of entities for which ALL specified components are present                      |
|Components...  |std::vector<EntityID>      |getEntitiesByComponentsAnyOf();    |                           |                                                   |Returns a list of entities for which ANY specified component is present                        |
|Component      |void                       |registerComponent();               |                           |                                                   |Register a component to the ECS                                                                |
|Component      |bool                       |componentExists();                 |                           |                                                   |Checks if a component is registered                                                            |
|Component      |bool                       |entityHasComponent();              |                           |UnregisteredComponent                              |Checks if an entity has a component attached                                                   |
|Component      |Component &                |entityAddComponent();              |EntityID                   |UnregisteredComponent, ComponentAlreadyAttached    |Adds a component to an entity                                                                  |
|Component      |Component &                |entityGetComponent();              |EntityID                   |UnregisteredComponent, ComponentNotAttached        |Returns a reference to a component attached to the entity                                      |
|Component      |void                       |entityRemoveComponent();           |EntityID                   |UnregisteredComponent, ComponentNotAttached        |Removes the component attached to the entity                                                   |
|Component      |ComponentPool<Component> & |getPool                            |                           |UnregisteredComponent                              |Returns the ComponentPool class of said component                                              |
|SystemClass    |SystemID                   |addSystem();                       |(int)                      |                                                   |Adds a system to the ECS and returns its id                                                    |
|               |void                       |toggleSystem();                    |SystemID                   |                                                   |Toggle on or off the specified system                                                          |
|               |bool                       |systemIsEnabled();                 |SystemID                   |                                                   |Checks if the system is enabled or not                                                         |
|               |                           |Update();                          |                           |                                                   |Calls the Update() method of every system that match the prerequisites (enabled and tickable)  |

## Components

Components are probably the most important aspect of an ECS. In this ecs, a component can be anything (a class, a struct, a typedef etc...) as long as a default constructor exists for that type (which is nearly everything in C++)
```cpp
struct comp_a {};
class comp_b {
    public:
        comp_b() {};
};
typedef std::size_t comp_c;
using comp_d = std::string;
```
Before using any component when the program runs, you have to register them to you ECS instance:
```cpp
ECS::ECS ecs;
ecs.registerComponent<comp_a>();
ecs.registerComponent<comp_b>();
ecs.registerComponent<comp_c>();
ecs.registerComponent<comp_d>();
```
If you fail to do that, you'll have an exception thrown when you'll try to access anything regarding an unregistered component

Components are used only by using their types, as a result, any entity can only have a single component of each type. Components and Entities are maped together inside the code, a single ID (EntityID) can get you access to every component attached to that entity.

Components are stored in a ComponentPool of their own type, it is not to the user to access such pool directly however if you have studied the code structure deeply enough, you are still given the option to get the raw pool structure:

```cpp
// componentPoolData is defined in Component.hpp
template<typename T>
using componentPoolData = std::map<EntityID, T*>;

// ComponentPool class
ComponentPool<comp_a> &comp_pool = ecs.getPool<comp_a>();

// Component pool map
componentPoolData<comp_a> &comp_map = comp_pool.getPool();
```
#### Be careful ! Component pool maps store components with raw pointers !

## Entities

Entities are represented by an ID, it is referring to a single entity, however if that entity is destroyed and new entities are created, beware that this ID might now refer to a new, different entity.

IDs are recycled throughout the execution, limiting the growth of IDs.

### Using components

You can create an entity and get its ID by calling the following method of the ECS class:
```cpp
ECS::ECS ecs;
ECS::EntityID entity = ecs.entityCreate();
```
Your `entity` now exists within the program as a simple ID. You can manipulate that `entity` by adding or requesting `components`
```cpp
using exampleComponent = std::size_t;

// Remember to register a component before using it
ecs.registerComponent<exampleComponent>();

// the entityAddComponent() method returns a reference to the newly created component
exampleComponent &comp = ecs.entityAddComponent<exampleComponent>(entity);
ecs.entityRemoveComponent<exampleComponent>(entity); // Removes the component from the entity
```
Using multiple components:
```cpp
using compA = std::size_t;
using compB = std::size_t;

ECS::ECS ecs;
ecs.registerComponent<compA>();
ecs.registerComponent<compB>();

ECS::EntityID e_1 = ecs.entityCreate();
ECS::EntityID e_2 = ecs.entityCreate();
ECS::EntityID e_3 = ecs.entityCreate();
ECS::EntityID e_4 = ecs.entityCreate();

ecs.entityAddComponent<compA>(e_1);
ecs.entityAddComponent<compB>(e_1);

ecs.entityAddComponent<compA>(e_2);
ecs.entityAddComponent<compA>(e_3);
ecs.entityAddComponent<compA>(e_4);

std::vector<ECS::EntityID> comp_a_and_b = ecs.getEntitiesByComponentsAllOf<compA, compB>(); // [e_1]
std::vector<ECS::EntityID> comp_a = ecs.getEntitiesByComponentsAllOf<compA>(); // [e_2, e_3, e_4]
std::vector<ECS::EntityID> comp_a_or_b = ecs.getEntitiesByComponentsAnyOf<compA, compB>(); // [e_1, e_2, e_3, e_4]
```
You can also checks if an entity has a component:
```cpp
bool has_comp = ecs.entityHasComponent<Component>(entity);
```

### Using Entity Groups
You can put any entity into a group, you'll have to modify the EntityGroup enum to add you own groups to the system.

Here's an example usage of groups:
```cpp
ECS::ECS ecs;
ECS::EntityID e_1 = ecs.entityCreate();
ECS::EntityID e_2 = ecs.entityCreate();
ECS::EntityID e_3 = ecs.entityCreate();
ECS::EntityID e_4 = ecs.entityCreate();
ECS::EntityID e_5 = ecs.entityCreate();

ecs.entitySetGroup(e_1, ECS::EntityGroup::EXAMPLES);
ecs.entitySetGroup(e_4, ECS::EntityGroup::EXAMPLES);

// Vector of EntityID
std::vector<ECS::EntityID> example_group; // []

example_group = ecs.getEntityGroup(ECS::EntityGroup::EXAMPLES); // [e_1, e_4]

ecs.entitySetGroup(e_1, ECS::EntityGroup::NONE);

example_group = ecs.getEntityGroup(ECS::EntityGroup::EXAMPLES); // [e_4]
```

## Systems

Using systems is pretty simple, you can create your custom systems by deriving the ISystem interface:

```cpp
class MySystem : public ECS::ISystem {
    public:
        MySystem();
        ~MySystem();

        void Update(ECS::ECS &, ECS::SystemID) override;
}
```

Then in the Update() method you can perform actions using the ecs reference:
```cpp
MySystem::Update(ECS::ECS &ecs, ECS::SystemID thisID)
{
    std::vector<ECS::EntityID> entities = ecs.getEntitiesByComponentsAllOf<exampleComponent>();

    // Do something...
}
```
To add your system to the ecs you can do so like that:
```cpp
ECS::SystemID sys_id = ecs.addSystem<MySystem>(/*Optional: tickrate*/);
```
You can toggle your system to turn it on/off during runtime and check if it is enabled or not:
```cpp
ecs.toggleSystem(sys_id);
bool is_on = ecs.systemIsEnabled(sys_id);
```
Each system that are enabled will have their Update() method called when the ECS's Update() method is called and that they are enabled and their tick is matching. (a 0 tickrate system will be cycled every tick)
