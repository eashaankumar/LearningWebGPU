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

void Game::update()
{
    std::cout << "Game Update " << time::timeSinceStart << std::endl;
}