#ifndef LP_GUI_GLFW_H
#define LP_GUI_GLFW_H

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef void(*LpGuiCB)(void);
typedef void(*LpGuiKeyCB)(int);
typedef void(*LpGuiMouseCB)(int,int,int);
typedef struct{float x,y,z,w,r,g,b,a,nx,ny,nz,nw,u,v,_p0,_p1;}LpGuiVertex; /* 64B */
typedef struct{float m[16];}LpGuiMat4;

static GLFWwindow* _lp_glfw_window = NULL;
static int _lp_glfw_w = 640;
static int _lp_glfw_h = 480;
static int _lp_keys[256] = {0};
static int _lp_mx = 0, _lp_my = 0;
static double _lp_scroll = 0;

static LpGuiCB _lp_on_timer = NULL;
static LpGuiKeyCB _lp_on_kdown = NULL, _lp_on_kup = NULL;
static LpGuiMouseCB _lp_on_mmove = NULL, _lp_on_mclick = NULL;

static LpGuiVertex _lp_gui_batch[262144]; static int _lp_gui_batch_n=0,_lp_gui_mode=0,_lp_gui_topology=0;
static LpGuiVertex _lp_gui_quad[4]; static int _lp_gui_quad_n=0;
static LpGuiVertex _lp_gui_mesh_cache[262144]; static int _lp_gui_mesh_cache_n=0;
static float _lp_gui_cr=1.0f,_lp_gui_cg=1.0f,_lp_gui_cb=1.0f,_lp_gui_ca=1.0f;
static float _lp_gui_nx=0,_lp_gui_ny=1.0f,_lp_gui_nz=0,_lp_gui_tu=0,_lp_gui_tv=0;
static int _lp_gui_lighting=0,_lp_gui_texturing=0;
static float _lp_gui_lx=3,_lp_gui_ly=4,_lp_gui_lz=5,_lp_gui_lr=1,_lp_gui_lg=1,_lp_gui_lb=1;
static float _lp_gui_ambient=0.2f;

static LpGuiMat4 _lp_gui_model,_lp_gui_view,_lp_gui_proj,_lp_gui_stack[32];static int _lp_gui_stack_n=0,_lp_gui_mat_init=0;

static void _lp_m4_identity(LpGuiMat4*m){memset(m,0,sizeof(*m));m->m[0]=m->m[5]=m->m[10]=m->m[15]=1.0f;}
static void _lp_m4_init(void){if(_lp_gui_mat_init)return;_lp_m4_identity(&_lp_gui_model);_lp_m4_identity(&_lp_gui_view);_lp_m4_identity(&_lp_gui_proj);_lp_gui_mat_init=1;}
static LpGuiMat4 _lp_m4_mul(LpGuiMat4 a,LpGuiMat4 b){LpGuiMat4 r;for(int c=0;c<4;c++)for(int row=0;row<4;row++)r.m[c*4+row]=a.m[0*4+row]*b.m[c*4+0]+a.m[1*4+row]*b.m[c*4+1]+a.m[2*4+row]*b.m[c*4+2]+a.m[3*4+row]*b.m[c*4+3];return r;}
static LpGuiMat4 _lp_m4_translate(float x,float y,float z){LpGuiMat4 m;_lp_m4_identity(&m);m.m[12]=x;m.m[13]=y;m.m[14]=z;return m;}
static LpGuiMat4 _lp_m4_scale(float x,float y,float z){LpGuiMat4 m;_lp_m4_identity(&m);m.m[0]=x;m.m[5]=y;m.m[10]=z;return m;}
static LpGuiMat4 _lp_m4_rotate(float a,float x,float y,float z){LpGuiMat4 m;_lp_m4_identity(&m);float l=sqrtf(x*x+y*y+z*z);if(l<=0.00001f)return m;x/=l;y/=l;z/=l;float c=cosf(a*0.01745329252f),s=sinf(a*0.01745329252f),t=1.0f-c;m.m[0]=t*x*x+c;m.m[4]=t*x*y-s*z;m.m[8]=t*x*z+s*y;m.m[1]=t*x*y+s*z;m.m[5]=t*y*y+c;m.m[9]=t*y*z-s*x;m.m[2]=t*x*z-s*y;m.m[6]=t*y*z+s*x;m.m[10]=t*z*z+c;return m;}
static LpGuiMat4 _lp_m4_perspective(float fov,float aspect,float zn,float zf){LpGuiMat4 m;memset(&m,0,sizeof(m));float f=1.0f/tanf(fov*0.00872664626f);m.m[0]=f/aspect;m.m[5]=f;m.m[10]=(zf+zn)/(zn-zf);m.m[11]=-1.0f;m.m[14]=(2.0f*zf*zn)/(zn-zf);return m;}
static LpGuiMat4 _lp_m4_ortho(float l,float r,float b,float t,float n,float f){LpGuiMat4 m;_lp_m4_identity(&m);m.m[0]=2.0f/(r-l);m.m[5]=2.0f/(t-b);m.m[10]=-2.0f/(f-n);m.m[12]=-(r+l)/(r-l);m.m[13]=-(t+b)/(t-b);m.m[14]=-(f+n)/(f-n);return m;}
static LpGuiMat4 _lp_m4_lookat(float ex,float ey,float ez,float cx,float cy,float cz,float ux,float uy,float uz){float fx=cx-ex,fy=cy-ey,fz=cz-ez;float fl=sqrtf(fx*fx+fy*fy+fz*fz);if(fl<=0.00001f)fl=1.0f;fx/=fl;fy/=fl;fz/=fl;float ul=sqrtf(ux*ux+uy*uy+uz*uz);if(ul<=0.00001f){ux=0;uy=1;uz=0;ul=1;}ux/=ul;uy/=ul;uz/=ul;float sx=fy*uz-fz*uy,sy=fz*ux-fx*uz,sz=fx*uy-fy*ux;float sl=sqrtf(sx*sx+sy*sy+sz*sz);if(sl<=0.00001f)sl=1.0f;sx/=sl;sy/=sl;sz/=sl;float vx=sy*fz-sz*fy,vy=sz*fx-sx*fz,vz=sx*fy-sy*fx;LpGuiMat4 m;_lp_m4_identity(&m);m.m[0]=sx;m.m[4]=sy;m.m[8]=sz;m.m[1]=vx;m.m[5]=vy;m.m[9]=vz;m.m[2]=-fx;m.m[6]=-fy;m.m[10]=-fz;m.m[12]=-(sx*ex+sy*ey+sz*ez);m.m[13]=-(vx*ex+vy*ey+vz*ez);m.m[14]=(fx*ex+fy*ey+fz*ez);return m;}
static LpGuiVertex _lp_gui_make_vertex(float x,float y,float z){_lp_m4_init();LpGuiMat4 mv=_lp_m4_mul(_lp_gui_view,_lp_gui_model);LpGuiMat4 mvp=_lp_m4_mul(_lp_gui_proj,mv);LpGuiVertex v;v.x=mvp.m[0]*x+mvp.m[4]*y+mvp.m[8]*z+mvp.m[12];v.y=mvp.m[1]*x+mvp.m[5]*y+mvp.m[9]*z+mvp.m[13];v.z=mvp.m[2]*x+mvp.m[6]*y+mvp.m[10]*z+mvp.m[14];v.w=mvp.m[3]*x+mvp.m[7]*y+mvp.m[11]*z+mvp.m[15];v.r=_lp_gui_cr;v.g=_lp_gui_cg;v.b=_lp_gui_cb;v.a=_lp_gui_ca;v.nx=_lp_gui_nx;v.ny=_lp_gui_ny;v.nz=_lp_gui_nz;v.nw=0;v.u=_lp_gui_tu;v.v=_lp_gui_tv;v._p0=v._p1=0;return v;}
static void _lp_gui_batch_add(LpGuiVertex v){if(_lp_gui_batch_n<(int)(sizeof(_lp_gui_batch)/sizeof(_lp_gui_batch[0])))_lp_gui_batch[_lp_gui_batch_n++]=v;}
static void _lp_gui_emit_vertex(float x,float y,float z){
    LpGuiVertex v;v.x=x;v.y=y;v.z=z;v.w=1.0f;
    v.r=_lp_gui_cr;v.g=_lp_gui_cg;v.b=_lp_gui_cb;v.a=_lp_gui_ca;
    v.nx=_lp_gui_nx;v.ny=_lp_gui_ny;v.nz=_lp_gui_nz;v.nw=0;
    v.u=_lp_gui_tu;v.v=_lp_gui_tv;v._p0=v._p1=0;
    if(_lp_gui_mode==1){_lp_gui_quad[_lp_gui_quad_n++]=v;if(_lp_gui_quad_n==4){_lp_gui_batch_add(_lp_gui_quad[0]);_lp_gui_batch_add(_lp_gui_quad[1]);_lp_gui_batch_add(_lp_gui_quad[2]);_lp_gui_batch_add(_lp_gui_quad[0]);_lp_gui_batch_add(_lp_gui_quad[2]);_lp_gui_batch_add(_lp_gui_quad[3]);_lp_gui_quad_n=0;}}else _lp_gui_batch_add(v);
}

static int _lp_glfw_map_key(int glfw_key) {
    if (glfw_key >= GLFW_KEY_A && glfw_key <= GLFW_KEY_Z) return glfw_key; /* Matches VK_A .. VK_Z */
    if (glfw_key >= GLFW_KEY_0 && glfw_key <= GLFW_KEY_9) return glfw_key; /* Matches VK_0 .. VK_9 */
    switch (glfw_key) {
        case GLFW_KEY_SPACE: return 0x20;
        case GLFW_KEY_ENTER: return 0x0D;
        case GLFW_KEY_ESCAPE: return 0x1B;
        case GLFW_KEY_UP: return 0x26;
        case GLFW_KEY_DOWN: return 0x28;
        case GLFW_KEY_LEFT: return 0x25;
        case GLFW_KEY_RIGHT: return 0x27;
        case GLFW_KEY_LEFT_SHIFT: case GLFW_KEY_RIGHT_SHIFT: return 0x10;
        case GLFW_KEY_LEFT_CONTROL: case GLFW_KEY_RIGHT_CONTROL: return 0x11;
        case GLFW_KEY_TAB: return 0x09;
        case GLFW_KEY_BACKSPACE: return 0x08;
    }
    return glfw_key & 0xFF;
}

static void _glfw_key_cb(GLFWwindow* window, int key, int scancode, int action, int mods) {
    int vk = _lp_glfw_map_key(key);
    if (action == GLFW_PRESS) {
        _lp_keys[vk & 0xFF] = 1;
        if (_lp_on_kdown) _lp_on_kdown(vk);
    } else if (action == GLFW_RELEASE) {
        _lp_keys[vk & 0xFF] = 0;
        if (_lp_on_kup) _lp_on_kup(vk);
    }
}
static void _glfw_cursor_cb(GLFWwindow* window, double xpos, double ypos) {
    _lp_mx = (int)xpos; _lp_my = (int)ypos;
    if (_lp_on_mmove) _lp_on_mmove(_lp_mx, _lp_my, 0);
}
static void _glfw_mouse_btn_cb(GLFWwindow* window, int button, int action, int mods) {
    int btn_id = 1;
    if (button == GLFW_MOUSE_BUTTON_RIGHT) btn_id = 2;
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) btn_id = 3;
    
    if (action == GLFW_PRESS) {
        _lp_keys[btn_id] = 1; /* Win32 VK_LBUTTON is 1, RBUTTON is 2, MBUTTON is 4 */
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) _lp_keys[4] = 1;
        if (_lp_on_mclick) _lp_on_mclick(_lp_mx, _lp_my, btn_id);
    } else if (action == GLFW_RELEASE) {
        _lp_keys[btn_id] = 0;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) _lp_keys[4] = 0;
    }
}
static void _glfw_scroll_cb(GLFWwindow* window, double xoffset, double yoffset) {
    _lp_scroll += yoffset;
}
static void _glfw_resize_cb(GLFWwindow* window, int width, int height) {
    _lp_glfw_w = width;
    _lp_glfw_h = height;
    glViewport(0, 0, width, height);
}

/* GUI Window & App Flow */
static inline int lp_gui_rgb(int r,int g,int b){return (r&0xFF)|((g&0xFF)<<8)|((b&0xFF)<<16);}
static inline void lp_gui_on_timer(LpGuiCB cb){_lp_on_timer=cb;}
static inline void lp_gui_on_key_down(LpGuiKeyCB cb){_lp_on_kdown=cb;}
static inline void lp_gui_on_key_up(LpGuiKeyCB cb){_lp_on_kup=cb;}
static inline void lp_gui_on_mouse_move(LpGuiMouseCB cb){_lp_on_mmove=cb;}
static inline void lp_gui_on_mouse_click(LpGuiMouseCB cb){_lp_on_mclick=cb;}
static inline int lp_gui_get_key_state(int k){return _lp_keys[k&0xFF];}
static inline int lp_gui_get_mouse_x(void){return _lp_mx;}
static inline int lp_gui_get_mouse_y(void){return _lp_my;}
static inline int lp_gui_get_tick_count(void){return (int)(glfwGetTime() * 1000.0);}
static inline int lp_gui_get_canvas_w(int cv){return _lp_glfw_w;}
static inline int lp_gui_get_canvas_h(int cv){return _lp_glfw_h;}
static int _lp_timer_ms = 16;
static inline int lp_gui_set_timer(int h,int ms){_lp_timer_ms=ms>0?ms:16;return 1;}
static inline int lp_gui_set_fps(int h,int fps){return lp_gui_set_timer(h,fps>0?1000/fps:16);}
static inline int lp_gui_get_scroll(void){
    double s = _lp_scroll;
    _lp_scroll = 0; /* Reset after reading */
    return (int)s;
}

static inline int lp_gui_window(const char*t,int w,int h){
    if(!glfwInit()){fprintf(stderr, "[LP] GLFW init failed.\n"); return 0;}
    _lp_glfw_w = w > 0 ? w : 640;
    _lp_glfw_h = h > 0 ? h : 480;
    _lp_glfw_window = glfwCreateWindow(_lp_glfw_w, _lp_glfw_h, t?t:"LP GUI", NULL, NULL);
    if(!_lp_glfw_window){glfwTerminate(); return 0;}
    glfwMakeContextCurrent(_lp_glfw_window);
    glfwSwapInterval(1); /* Enable VSync */

    glfwSetKeyCallback(_lp_glfw_window, _glfw_key_cb);
    glfwSetCursorPosCallback(_lp_glfw_window, _glfw_cursor_cb);
    glfwSetMouseButtonCallback(_lp_glfw_window, _glfw_mouse_btn_cb);
    glfwSetScrollCallback(_lp_glfw_window, _glfw_scroll_cb);
    glfwSetFramebufferSizeCallback(_lp_glfw_window, _glfw_resize_cb);
    
    return 1; /* Return a non-zero handle */
}

static inline void lp_gui_close(int h){
    if(_lp_glfw_window) glfwSetWindowShouldClose(_lp_glfw_window, GLFW_TRUE);
}
static inline void lp_gui_set_title(int h,const char*t){
    if(_lp_glfw_window) glfwSetWindowTitle(_lp_glfw_window, t?t:"");
}

static inline void lp_gui_run_game(int h){
    double last_time = glfwGetTime();
    while(!glfwWindowShouldClose(_lp_glfw_window)){
        glfwPollEvents();
        double current_time = glfwGetTime();
        if ((current_time - last_time) * 1000.0 >= _lp_timer_ms) {
            if(_lp_on_timer) _lp_on_timer();
            last_time = current_time;
        }
    }
    glfwTerminate();
}

/* Mouse Capture (GLFW raw input) */
static inline void lp_gui_set_mouse_capture(int captured) {
    if(!_lp_glfw_window) return;
    if(captured) glfwSetInputMode(_lp_glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    else glfwSetInputMode(_lp_glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

static inline int lp_gui_canvas3d(int p,int x,int y,int w,int h){
    if (!_lp_glfw_window) return 0;
    glEnable(GL_DEPTH_TEST);
    glClearColor(0,0,0,1);
    glViewport(0,0,_lp_glfw_w,_lp_glfw_h);
    _lp_m4_init();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /* To use gluPerspective we need GLU, which usually links nicely, but for simplicity: */
    float fov = 45.0f, aspect = (float)_lp_glfw_w / (float)_lp_glfw_h;
    float zNear = 0.1f, zFar = 1000.0f;
    float fH = tanf((float)(fov / 360.0f * 3.14159f)) * zNear;
    float fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, zNear, zFar);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    return 1;
}

static inline void lp_gui_clear3d(int cv,double r,double g,double b){
    glClearColor((float)r,(float)g,(float)b,1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

static inline void lp_gui_present3d(int cv){
    glfwSwapBuffers(_lp_glfw_window);
}

static inline void lp_gui_3d_begin(void){ _lp_gui_batch_n=0; }

static inline void lp_gui_3d_end(void){
    if(_lp_gui_batch_n <= 0) return;
    glBegin(GL_TRIANGLES);
    for(int i=0; i<_lp_gui_batch_n; i++) {
        LpGuiVertex *v = &_lp_gui_batch[i];
        glColor4f(v->r, v->g, v->b, v->a);
        glVertex3f(v->x, v->y, v->z);
    }
    glEnd();
}

static inline void lp_gui_color(double r,double g,double b,double a){_lp_gui_cr=(float)r;_lp_gui_cg=(float)g;_lp_gui_cb=(float)b;_lp_gui_ca=(float)a;}
static inline void lp_gui_mode(int m){_lp_gui_mode=m;}
static inline void lp_gui_vertex(double x,double y,double z){_lp_gui_emit_vertex((float)x,(float)y,(float)z);}
static inline void lp_gui_normal(double x,double y,double z){_lp_gui_nx=(float)x;_lp_gui_ny=(float)y;_lp_gui_nz=(float)z;}
static inline void lp_gui_texcoord(double u,double v){_lp_gui_tu=(float)u;_lp_gui_tv=(float)v;}

static inline void lp_gui_load_identity(void){_lp_m4_identity(&_lp_gui_model);}
static inline void lp_gui_push_matrix(void){if(_lp_gui_stack_n<32)_lp_gui_stack[_lp_gui_stack_n++]=_lp_gui_model;}
static inline void lp_gui_pop_matrix(void){if(_lp_gui_stack_n>0)_lp_gui_model=_lp_gui_stack[--_lp_gui_stack_n];}
static inline void lp_gui_translate(double x,double y,double z){_lp_gui_model=_lp_m4_mul(_lp_m4_translate((float)x,(float)y,(float)z),_lp_gui_model);}
static inline void lp_gui_scale(double x,double y,double z){_lp_gui_model=_lp_m4_mul(_lp_m4_scale((float)x,(float)y,(float)z),_lp_gui_model);}
static inline void lp_gui_rotate(double ang,double x,double y,double z){_lp_gui_model=_lp_m4_mul(_lp_m4_rotate((float)ang,(float)x,(float)y,(float)z),_lp_gui_model);}

static inline void lp_gui_camera(double ex,double ey,double ez,double cx,double cy,double cz,double ux,double uy,double uz){
    _lp_gui_view=_lp_m4_lookat((float)ex,(float)ey,(float)ez,(float)cx,(float)cy,(float)cz,(float)ux,(float)uy,(float)uz);
}

// Dummy impls for win32 specific components
static inline int lp_gui_label(int p,const char*t,int x,int y){return 0;}
static inline int lp_gui_button(int p,const char*t,int x,int y,int w,int h){return 0;}
// ... Add minimal stubs for the rest if accessed ... 

#endif
