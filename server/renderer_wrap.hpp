
/*
 * A way to implement a wrapper over the base enabler->renderer.
 * Stolen from rendermax/renderer_opengl.hpp, although it looks like
 * Stonesense has almost exactly the same code for its overlay.
 * TODO: turn into a proper header/cpp, make it a proper dfhack module
 */
struct renderer_wrap : public renderer {
private:
    void set_to_null() {
        screen = NULL;
        screentexpos = NULL;
        screentexpos_addcolor = NULL;
        screentexpos_grayscale = NULL;
        screentexpos_cf = NULL;
        screentexpos_cbr = NULL;
        screen_old = NULL;
        screentexpos_old = NULL;
        screentexpos_addcolor_old = NULL;
        screentexpos_grayscale_old = NULL;
        screentexpos_cf_old = NULL;
        screentexpos_cbr_old = NULL;
    }
    void copy_from_inner() {
        screen = parent->screen;
        screentexpos = parent->screentexpos;
        screentexpos_addcolor = parent->screentexpos_addcolor;
        screentexpos_grayscale = parent->screentexpos_grayscale;
        screentexpos_cf = parent->screentexpos_cf;
        screentexpos_cbr = parent->screentexpos_cbr;
        screen_old = parent->screen_old;
        screentexpos_old = parent->screentexpos_old;
        screentexpos_addcolor_old = parent->screentexpos_addcolor_old;
        screentexpos_grayscale_old = parent->screentexpos_grayscale_old;
        screentexpos_cf_old = parent->screentexpos_cf_old;
        screentexpos_cbr_old = parent->screentexpos_cbr_old;
    }
    void copy_to_inner() {
        parent->screen = screen;
        parent->screentexpos = screentexpos;
        parent->screentexpos_addcolor = screentexpos_addcolor;
        parent->screentexpos_grayscale = screentexpos_grayscale;
        parent->screentexpos_cf = screentexpos_cf;
        parent->screentexpos_cbr = screentexpos_cbr;
        parent->screen_old = screen_old;
        parent->screentexpos_old = screentexpos_old;
        parent->screentexpos_addcolor_old = screentexpos_addcolor_old;
        parent->screentexpos_grayscale_old = screentexpos_grayscale_old;
        parent->screentexpos_cf_old = screentexpos_cf_old;
        parent->screentexpos_cbr_old = screentexpos_cbr_old;
    }
public:
    renderer_wrap(renderer* parent):parent(parent)
    {
        copy_from_inner();
    }
    virtual void update_tile(int32_t x, int32_t y) {
        copy_to_inner();
        parent->update_tile(x,y);
    };
    virtual void update_all() {
        copy_to_inner();
        parent->update_all();
    };
    virtual void render() {
        copy_to_inner();
        parent->render();
    };
    virtual void set_fullscreen() {
        copy_to_inner();
        parent->set_fullscreen();
        copy_from_inner();
    };
    virtual void zoom(df::zoom_commands z) {
        copy_to_inner();
        parent->zoom(z);
        copy_from_inner();
    };
    virtual void resize(int32_t w, int32_t h) {
        copy_to_inner();
        parent->resize(w,h);
        copy_from_inner();
    };
    virtual void grid_resize(int32_t w, int32_t h) {
        copy_to_inner();
        parent->grid_resize(w,h);
        copy_from_inner();
    };
    virtual ~renderer_wrap() {
        df::global::enabler->renderer=parent;
    };
    virtual bool get_mouse_coords(int32_t* x, int32_t* y) {
        return parent->get_mouse_coords(x,y);
    };
    virtual bool uses_opengl() {
        return parent->uses_opengl();
    };
    void invalidateRect(int32_t x,int32_t y,int32_t w,int32_t h)
    {
        for(int i=x;i<x+w;i++)
            for(int j=y;j<y+h;j++)
            {
                int index=i*df::global::gps->dimy + j;
                screen_old[index*4]=screen[index*4]+1;//ensure tile is different
            }
    };
    void invalidate()
    {
        invalidateRect(0,0,df::global::gps->dimx,df::global::gps->dimy);
        //df::global::gps->force_full_display_count++;
    };
protected:
    renderer* parent;
};
