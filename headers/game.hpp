#ifndef GAME
#define GAME
#include <entt/entt.hpp>
#include<glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

namespace engine::game
{
    class Game
    {
        public:
            Game();
            ~Game();
            void update();
        private:
            entt::registry m_registry;
    };

    struct TransformComponent{
        glm::mat4 transform;

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::mat4& _transform)
            : transform(_transform){}

        operator glm::mat4&() {return transform;}
        operator const glm::mat4&() {return transform;}
    };
}
#endif