#include "PlayLevel.h"
#include "res.h"

using namespace std;

void PlayLevel(sf::RenderWindow* window, const char * l_name)
{
    wwd_map level(l_name);

    string path = level.getLevelDir();
    path += "\\SCREENS\\LOADING.JPG";

    sf::Texture bgd; bgd.loadFromFile(path.c_str());
    sf::RectangleShape loading_s((sf::Vector2f)window->getSize());
    loading_s.setTexture(&bgd);

    window->draw(loading_s);
    window->display();

    path = level.getLevelDir();
    path += "\\IMAGES\\";
    level.loadResource(path.c_str(), "LEVEL");

    level.loadResource("DATA\\CLAW\\IMAGES\\", "CLAW");
    level.loadResource("DATA\\GAME\\IMAGES\\", "GAME");

    sf::Font font;
    if (!font.loadFromFile("cambriab.ttf"))
        return;

    sf::Text text;
    text.setFont(font);
    text.setString(to_string(level.spawn_x)+"x"+to_string(level.spawn_y));
    text.setCharacterSize(24);
    text.setColor(sf::Color::White);
    text.setStyle(sf::Text::Bold);
    text.setPosition(sf::Vector2f(level.spawn_x, level.spawn_y));

    sf::Sprite claw;
    wwd_resource * clawres = level.getResource("CLAW");
    sf::Texture c_tex;
    c_tex.loadFromImage(clawres->texture);
    claw.setTexture(c_tex);
    claw.setTextureRect(clawres->get(2));
    claw.setPosition(level.spawn_x, level.spawn_y);

    sf::View view(sf::FloatRect(0, 0, window->getSize().x, window->getSize().y));

    sf::Vector2f s = view.getCenter();

    view.setCenter(sf::Vector2f(level.spawn_x, level.spawn_y));
    window->setView(view);

    sf::Vector2i touch_st_pos; sf::Vector2f touch_center;
    bool touch_moving = false; unsigned int touch_finger=0;

    while (window->isOpen())
    {
        sf::Event event;
        while (window->pollEvent(event))
        {
            switch(event.type)
            {
                case sf::Event::Closed:
                    window->close();
                break;
                case sf::Event::Resized:
                    view.setSize(event.size.width, event.size.height);
                    window->setView(sf::View(view));
                break;
                case sf::Event::TouchBegan:
                    if(!touch_moving)
                    {
                        touch_finger = event.touch.finger;
                        touch_st_pos.x = event.touch.x;
                        touch_st_pos.y = event.touch.y;
                        touch_center = view.getCenter();
                        touch_moving = true;
                    }
                break;
                case sf::Event::TouchEnded:
                    if(event.touch.finger==touch_finger)
                        touch_moving = false;
                break;
                case sf::Event::KeyPressed:
                    if(event.key.code == sf::Keyboard::Escape)
                        return;
                break;
                default:
                break;
            }


        }

        if(touch_moving && sf::Touch::isDown(touch_finger))
        {
            sf::Vector2i touch_cur_pos = sf::Touch::getPosition(touch_finger, *window);
            view.setCenter(touch_center);
            sf::Vector2f diff = (sf::Vector2f)(touch_st_pos-touch_cur_pos);
            view.move(diff);
        }


        #define step 9
        //sf::Touch::getPosition()
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            view.move(-step,0);
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            view.move(step,0);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
            view.move(0,-step);
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
            view.move(0,step);

        window->setView(view);

        window->clear();

        sf::Vector2f c = view.getCenter();
        level.draw( window, sf::IntRect( c.x-s.x, c.y-s.y, s.x*2, s.y*2 ) );
        window->draw(claw);
        window->draw(text);
        window->display();
    }

return;

}
