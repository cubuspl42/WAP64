#include "res.h"
#include <assert.h>
#include <string.h>
#include <fstream>

using namespace std;

unsigned getFirstNumber(const char* s)
{
    string str = s;

    for(string::iterator c = str.begin(); c!=str.end(); c++)
        if(*c >= '0' && *c <= '9') { return atoi(&*c); }
    return 0;
}

wwd_map::wwd_map(string filename){
    wwd = wap_wwd_create();

    assert(!wap_wwd_open(wwd, filename.c_str()));
    //assert(wap_wwd_get_properties(wwd)->flags & WAP_WWD_FLAG_COMPRESS);

    auto map_properties = wap_wwd_get_properties(wwd);

    base_level = getFirstNumber(map_properties->level_name);

    assert(base_level);

    string path = getLevelDir();
    path += "\\PALETTES\\MAIN.PAL";
    ifstream pal;
    pal.open(path.c_str(),ios::binary|ios::in);
    char c[3];
    for(int i=0; i<256; i++)
    {
        pal.read(c, 3);
        palette[i] = sf::Color(c[0], c[1], c[2], 255);
    }

    //path = getLevelDir();
    //path += "\\IMAGES\\";
    //loadResource(path.c_str(), "LEVEL");

    loadResource("DATA\\CLAW\\IMAGES\\", "CLAW");
    //loadResource("DATA\\GAME\\IMAGES\\", "GAME");

    spawn_x = map_properties->start_x;
    spawn_y = map_properties->start_y;

    for(uint32_t i=0; i<wap_wwd_get_plane_count(wwd); i++)
    {
        auto plane = wap_wwd_get_plane(wwd, i);
        planes.emplace_back(this, plane);

        if(wap_plane_get_properties(plane)->flags & WAP_PLANE_FLAG_MAIN_PLANE)
            main_plane = &*planes.end();
        //auto obj_c = wap_plane_get_object_count(plane);
        //cout << i << ": " << wap_plane_get_properties(plane)->name << "(" << wap_plane_get_image_set(plane, 0) << ")" << endl;
    }
    for(auto it = planes.begin(); it!=planes.end(); it++ )
    {
        it->texture.loadFromImage(it->tileset->texture);
        it->tile.setTexture(it->texture);
        if(it->plane_w*it->plane_h<4000)
            it->preRender();
    }

}

wwd_map::~wwd_map(){
    wap_wwd_free(wwd);
}

const char* wwd_map::getLevelName(){ return wap_wwd_get_properties(wwd)->level_name; }

wwd_map_plane* wwd_map::getMainPlane(){
    wwd_map_plane * main_plane = 0;

    for(auto i=planes.begin(); i!=planes.end(); i++)
    {
        if(wap_plane_get_properties(i->plane)->flags & WAP_PLANE_FLAG_MAIN_PLANE)
            main_plane = &*i;
        //auto obj_c = wap_plane_get_object_count(plane);
        //cout << i << ": " << wap_plane_get_properties(plane)->name << "(" << wap_plane_get_image_set(plane, 0) << ")" << endl;
    }
    return main_plane;
}

const char* wwd_map::getLevelDir()
{
    string path = "DATA\\LEVEL";
    path += to_string(base_level);
    return path.c_str();
}

wwd_resource* wwd_map::loadResource(const char* p, const char* as)
{
    string path = p;

    DIR * assets = opendir(p);
    struct dirent *d;
    if(assets)
    {
        while((d = readdir(assets)) != NULL)
            if (strcmp(d->d_name, ".") && strcmp(d->d_name, ".."))
            {
                DIR * dir = opendir((path+d->d_name).c_str());
                if (dir != NULL)
                {
                   closedir(dir);
                   string npath = path+d->d_name+'\\';
                   string nas = as;
                   nas += '_';
                   nas += d->d_name;
                   loadResource(npath.c_str(), nas.c_str());
                   continue;
                }

                unsigned short id = getFirstNumber(d->d_name);
                sf::Image img;
                assert(img.loadFromFile(path+d->d_name));
                resources[as].set(id, img);
            }
    }
    return &resources[as];
}

void wwd_map_plane::setTileImage(unsigned short id){
    tile.setTextureRect((*tileset)[id]);
}

uint32_t wwd_map_plane::getTile(uint32_t x, uint32_t y)
{
    uint32_t plane_w, plane_h;
    wap_plane_get_map_dimensions(plane, &plane_w, &plane_h);
    if(x>=plane_w)
    {
        if(wap_plane_get_properties(plane)->flags & WAP_PLANE_FLAG_X_WRAPPING)
            x= x % plane_w;
        else return 0;
    }
    if(y>=plane_h)
    {
        if(wap_plane_get_properties(plane)->flags & WAP_PLANE_FLAG_Y_WRAPPING)
            y= y % plane_h;
        else return 0;
    }
    if(x<0 || y<0) return 0;
    return wap_plane_get_tile(plane, x, y);
}

wwd_map_plane::wwd_map_plane(wwd_map * wp, wap_plane* p)
{
    wwd_map_ptr = wp;
    plane = p;
    TILE_W = wap_plane_get_properties(plane)->tile_width;
    TILE_H = wap_plane_get_properties(plane)->tile_height;
    //tile.setSize(sf::Vector2f(TILE_W, TILE_H));
    fill_color = wwd_map_ptr->palette[wap_plane_get_properties(plane)->fill_color];

    const char * imgset = wap_plane_get_image_set(plane, 0);
    tileset = wwd_map_ptr->getResource(imgset);
    if(!tileset)
    {
        //cerr << "not loaded" << endl;
        string path = wwd_map_ptr->getLevelDir();
        path += "\\TILES\\";
        path += imgset;
        path += '\\';
        tileset = wwd_map_ptr->loadResource( path.c_str(), imgset );
    }
    wap_plane_get_map_dimensions(plane,&plane_w,&plane_h);
}

void wwd_map_plane::preRender()
{
    preRendered = true;
    sf::RenderTexture preRendTex;
    preRendTex.create(plane_w*TILE_W, plane_h*TILE_H);
    for(uint32_t y=0; y<plane_h; y++ )
        for(uint32_t x=0; x<plane_w; x++ )
            if(auto tile_id = getTile(x, y))
            {
                if(tile_id == 4294967295) continue;
                if(tile_id == 4008636142)
                {
                    sf::RectangleShape t_fill(sf::Vector2f(TILE_W, TILE_H));
                    t_fill.setPosition((x*TILE_W), (y*TILE_H));
                    t_fill.setFillColor(fill_color);
                    preRendTex.draw(t_fill);
                }
                else
                {
                    tile.setPosition((x*TILE_W), (y*TILE_H));
                    setTileImage(tile_id);
                    preRendTex.draw(tile);
                }
            }
    preRendTex.display();
    texture = preRendTex.getTexture();
}

void wwd_map::draw(sf::RenderTarget& target, sf::IntRect rect)
{
    for(auto p = planes.begin(); p!=planes.end(); p++)
        p->draw( target, rect);
}

void wwd_map_plane::draw(sf::RenderTarget& target, sf::IntRect rect )
{

    int32_t m_x = wap_plane_get_properties(plane)->movement_x_percent;
    int32_t m_y = wap_plane_get_properties(plane)->movement_y_percent;

    //if(m_x==150) m_x = 50;
    //if(m_y==125) m_y = 125;

    float percent_x = m_x * 0.01;
    float percent_y = m_y * 0.01;

    if(preRendered)
    {
        sf::Sprite prPlane;
        prPlane.setTexture(texture);
        int px = (rect.left+320)*percent_x, py = (rect.top+240)*percent_y;
        prPlane.setOrigin(px%(plane_w*TILE_W)-320, py%(plane_h*TILE_H)-240);
        prPlane.setPosition(rect.left, rect.top);
        target.draw(prPlane);
        prPlane.move(plane_w*TILE_W, 0);
        target.draw(prPlane);
        prPlane.move(0, plane_h*TILE_H);
        target.draw(prPlane);
        prPlane.move(-plane_w*TILE_W, 0);
        target.draw(prPlane);
        return;
    }

    rect.left *= percent_x;
    rect.top *= percent_y;

    //tile.setTexture(&texture);

    int x0 = rect.left/TILE_W;
    int y0 = rect.top/TILE_H;
    int xmax = (rect.left+rect.width)/TILE_W+1/percent_x;
    int ymax = (rect.top+rect.height)/TILE_H+1/percent_y;

    int px=0, py=0;

    if(m_x<100)
    {
        if(m_x==75)
            px = (rect.left%TILE_W)/3;
        else
            px = rect.left%TILE_W;
    }
    else if(m_x>100)
    {
        if(m_x==150)
            px = -(rect.left%TILE_W)/3;
        else
            px = -(rect.left%TILE_W)*percent_x;
    }
    if(m_y<100)
    {
        if(m_y==75)
            py = rect.top%TILE_H/3;
        else
            py = rect.top%TILE_H;
    }
    else if(m_y>100)
    {
        if(m_y==125)
            py = -(rect.top%TILE_H)/5;
        else
            py = -(rect.top%TILE_H)/3;
    }


    for(int y=y0; y<=ymax; y++ )
        for(int x=x0; x<=xmax; x++ )
            if(auto tile_id = getTile(x, y))
            {
                if(tile_id == 4294967295) continue;
                tile.setPosition((x0*TILE_W)/percent_x+(x-x0)*TILE_W+px, (y0*TILE_H)/percent_y+(y-y0)*TILE_H+py);
                if(tile_id == 4008636142)
                {
                    tile.setTextureRect(sf::IntRect(0, 0, TILE_W, TILE_H));
                    //tile.setFillColor(fill_color);
                    target.draw(tile);
                    //tile.setFillColor(sf::Color::White);
                }
                else
                {
                    setTileImage(tile_id);
                    target.draw(tile);
                }
            }
}

sf::IntRect wwd_resource::operator[](unsigned short id)
{
    return imageset[id];
}

sf::IntRect wwd_resource::get(unsigned short id)
{
    return imageset[id];
}

void wwd_resource::set(unsigned short id, sf::Image &img)
{
    sf::IntRect place = imageset[id];
    if(!place.width)
    {
        place = sf::IntRect( texture.getSize().x, 0, img.getSize().x, img.getSize().y );
        sf::Image cp = texture;
        texture.create(texture.getSize().x+img.getSize().x, max(texture.getSize().y,img.getSize().y), sf::Color::Transparent );
        texture.copy(cp, 0, 0);
        imageset[id] = place;
    }
    texture.copy(img, place.left, place.top);
}

wwd_resource* wwd_map::getResource(const char* name)
{
    auto it = resources.find(name);
    if (it != resources.end())
        return &it->second;
    return 0;
}
