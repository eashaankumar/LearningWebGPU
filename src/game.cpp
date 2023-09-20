#include <iostream>


#include "game.hpp"
#include "time.hpp"

using namespace engine::game;

Game::Game()
{
    entt::entity entity = m_registry.create();
    m_registry.emplace<TransformComponent>(entity, glm::mat3(1.0f));
    
    if (m_registry.all_of<TransformComponent>(entity))
    {
        std::cout << "Found component in registry" << std::endl;
        TransformComponent& transform = m_registry.get<TransformComponent>(entity);
    }
}

Game::~Game()
{

}

void Game::update()
{
    auto view = m_registry.view<TransformComponent>();
    for(auto entity: view)
    {
        // update
        TransformComponent& transform = m_registry.get<TransformComponent>(entity);
    }

    std::cout << "Game Update " << time::timeSinceStart << std::endl;
}