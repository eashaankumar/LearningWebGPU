#include <iostream>
#include "game.hpp"
#include "time.hpp"

using namespace engine::game;

Game::Game()
{
}

Game::~Game()
{

}

void updateEntities(entt::registry& registry)
{
    auto view = registry.view<TransformComponent>();
    for(auto entity: view)
    {
        // update
        TransformComponent& transform = registry.get<TransformComponent>(entity);
    }
}

void addEntity(entt::registry& registry, TransformComponent transform)
{
    entt::entity entity = registry.create();
    registry.emplace<TransformComponent>(entity, transform);
}

void Game::update()
{
    addEntity(m_registry, glm::mat4(1.0f));
    updateEntities(m_registry);

    if ( (int)time::timeSinceStart % 5 == 0)
        std::cout << "Entities: " << m_registry.size() << " FPS: " << time::framesPerSecond << std::endl;
}