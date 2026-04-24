#ifndef LP_GUI_H
#define LP_GUI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define LP_GUI_BACKEND_AUTO 0
#define LP_GUI_BACKEND_OPENGL 1
#define LP_GUI_BACKEND_DIRECTX11 2
#define LP_GUI_BACKEND_VULKAN 3
#ifdef _WIN32
#ifndef COBJMACROS
#define COBJMACROS
#endif
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
/* Force discrete GPU on NVIDIA Optimus / AMD Switchable */
#ifdef __GNUC__
__attribute__((dllexport)) unsigned long NvOptimusEnablement = 1;
__attribute__((dllexport)) int AmdPowerXpressRequestHighPerformance = 1;
#else
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif
/* OpenGL */
#include <GL/gl.h>
#include <GL/glu.h>
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif
/* WGL VSync extension */
typedef BOOL(WINAPI*PFNWGLSWAPINTERVALEXT)(int);
static PFNWGLSWAPINTERVALEXT _lp_wglSwapInterval=NULL;
/* Types */
typedef void(*LpGuiCB)(void);
typedef void(*LpGuiKeyCB)(int);
typedef void(*LpGuiMouseCB)(int,int,int);
typedef struct{HWND hwnd;HDC memDC;HBITMAP memBmp,oldBmp;int w,h;}LpCanvas2D;
typedef struct{
    HWND hwnd;HDC hdc;HGLRC hglrc;int w,h;int backend;
    IDXGISwapChain*dx_swap;ID3D11Device*dx_device;ID3D11DeviceContext*dx_ctx;ID3D11RenderTargetView*dx_rtv;
    ID3D11DepthStencilView*dx_dsv;ID3D11DepthStencilState*dx_dss;
    ID3D11VertexShader*dx_vs;ID3D11PixelShader*dx_ps;ID3D11InputLayout*dx_layout;ID3D11Buffer*dx_vbuf;
}LpCanvas3D;
typedef struct{float x,y,z,w,r,g,b,a,nx,ny,nz,nw,u,v,_p0,_p1;}LpGuiVertex; /* 64B */
typedef struct{float m[16];}LpGuiMat4;
/* Globals */
#define LP_GUI_MAX 32
static HINSTANCE _lp_hInst=NULL;
static int _lp_cls_reg=0,_lp_gl_cls_reg=0;
static LpGuiCB _lp_on_timer=NULL;
static LpGuiKeyCB _lp_on_kdown=NULL,_lp_on_kup=NULL;
static LpGuiMouseCB _lp_on_mmove=NULL,_lp_on_mclick=NULL;
static LpCanvas2D _lp_c2d[LP_GUI_MAX];static int _lp_c2d_n=0;
static LpCanvas3D _lp_c3d[LP_GUI_MAX];static int _lp_c3d_n=0;
static int _lp_mx=0,_lp_my=0,_lp_keys[256]={0};
static int _lp_gui_backend_pref=LP_GUI_BACKEND_AUTO,_lp_gui_backend_active=LP_GUI_BACKEND_OPENGL,_lp_gui_vsync=1;
static char _lp_gui_dx_name[256]="",_lp_gui_dx_vendor[64]="Unknown",_lp_gui_vk_info[256]="Vulkan loader unavailable";
static LpCanvas3D*_lp_active3d=NULL;
static LpGuiVertex _lp_gui_batch[65536];static int _lp_gui_batch_n=0,_lp_gui_mode=0,_lp_gui_topology=0;
static LpGuiVertex _lp_gui_quad[4];static int _lp_gui_quad_n=0;
static float _lp_gui_cr=1.0f,_lp_gui_cg=1.0f,_lp_gui_cb=1.0f,_lp_gui_ca=1.0f;
static float _lp_gui_nx=0,_lp_gui_ny=1.0f,_lp_gui_nz=0,_lp_gui_tu=0,_lp_gui_tv=0;
static int _lp_gui_lighting=0,_lp_gui_texturing=0;
static float _lp_gui_lx=3,_lp_gui_ly=4,_lp_gui_lz=5,_lp_gui_lr=1,_lp_gui_lg=1,_lp_gui_lb=1;
static float _lp_gui_ambient=0.2f;
static ID3D11Buffer*_lp_gui_dx_cb=NULL;
static ID3D11SamplerState*_lp_gui_dx_sampler=NULL;
static ID3D11RasterizerState*_lp_gui_dx_rast=NULL;
typedef struct{float mvp[16];float lightPos[4];float lightCol[4];float ambient[4];int flags[4];}LpDxCB;
static LpGuiMat4 _lp_gui_model,_lp_gui_view,_lp_gui_proj,_lp_gui_stack[32];static int _lp_gui_stack_n=0,_lp_gui_mat_init=0;

/* Color */
static inline int lp_gui_rgb(int r,int g,int b){return(int)RGB(r,g,b);}
/* Canvas lookup */
static LpCanvas2D*_lp_fc2d(HWND h){for(int i=0;i<_lp_c2d_n;i++)if(_lp_c2d[i].hwnd==h)return&_lp_c2d[i];return NULL;}
static LpCanvas3D*_lp_fc3d(HWND h){for(int i=0;i<_lp_c3d_n;i++)if(_lp_c3d[i].hwnd==h)return&_lp_c3d[i];return NULL;}
static LRESULT CALLBACK _lp_gui_wndproc(HWND h,UINT m,WPARAM w,LPARAM l);
static void _lp_m4_identity(LpGuiMat4*m){ZeroMemory(m,sizeof(*m));m->m[0]=m->m[5]=m->m[10]=m->m[15]=1.0f;}
static void _lp_m4_init(void){if(_lp_gui_mat_init)return;_lp_m4_identity(&_lp_gui_model);_lp_m4_identity(&_lp_gui_view);_lp_m4_identity(&_lp_gui_proj);_lp_gui_mat_init=1;}
static LpGuiMat4 _lp_m4_mul(LpGuiMat4 a,LpGuiMat4 b){LpGuiMat4 r;for(int c=0;c<4;c++)for(int row=0;row<4;row++)r.m[c*4+row]=a.m[0*4+row]*b.m[c*4+0]+a.m[1*4+row]*b.m[c*4+1]+a.m[2*4+row]*b.m[c*4+2]+a.m[3*4+row]*b.m[c*4+3];return r;}
static LpGuiMat4 _lp_m4_translate(float x,float y,float z){LpGuiMat4 m;_lp_m4_identity(&m);m.m[12]=x;m.m[13]=y;m.m[14]=z;return m;}
static LpGuiMat4 _lp_m4_scale(float x,float y,float z){LpGuiMat4 m;_lp_m4_identity(&m);m.m[0]=x;m.m[5]=y;m.m[10]=z;return m;}
static LpGuiMat4 _lp_m4_rotate(float a,float x,float y,float z){LpGuiMat4 m;_lp_m4_identity(&m);float l=sqrtf(x*x+y*y+z*z);if(l<=0.00001f)return m;x/=l;y/=l;z/=l;float c=cosf(a*0.01745329252f),s=sinf(a*0.01745329252f),t=1.0f-c;m.m[0]=t*x*x+c;m.m[4]=t*x*y-s*z;m.m[8]=t*x*z+s*y;m.m[1]=t*x*y+s*z;m.m[5]=t*y*y+c;m.m[9]=t*y*z-s*x;m.m[2]=t*x*z-s*y;m.m[6]=t*y*z+s*x;m.m[10]=t*z*z+c;return m;}
static LpGuiMat4 _lp_m4_perspective(float fov,float aspect,float zn,float zf){LpGuiMat4 m;ZeroMemory(&m,sizeof(m));float f=1.0f/tanf(fov*0.00872664626f);m.m[0]=f/aspect;m.m[5]=f;m.m[10]=(zf+zn)/(zn-zf);m.m[11]=-1.0f;m.m[14]=(2.0f*zf*zn)/(zn-zf);return m;}
static LpGuiMat4 _lp_m4_ortho(float l,float r,float b,float t,float n,float f){LpGuiMat4 m;_lp_m4_identity(&m);m.m[0]=2.0f/(r-l);m.m[5]=2.0f/(t-b);m.m[10]=-2.0f/(f-n);m.m[12]=-(r+l)/(r-l);m.m[13]=-(t+b)/(t-b);m.m[14]=-(f+n)/(f-n);return m;}
static LpGuiMat4 _lp_m4_lookat(float ex,float ey,float ez,float cx,float cy,float cz,float ux,float uy,float uz){float fx=cx-ex,fy=cy-ey,fz=cz-ez;float fl=sqrtf(fx*fx+fy*fy+fz*fz);if(fl<=0.00001f)fl=1.0f;fx/=fl;fy/=fl;fz/=fl;float ul=sqrtf(ux*ux+uy*uy+uz*uz);if(ul<=0.00001f){ux=0;uy=1;uz=0;ul=1;}ux/=ul;uy/=ul;uz/=ul;float sx=fy*uz-fz*uy,sy=fz*ux-fx*uz,sz=fx*uy-fy*ux;float sl=sqrtf(sx*sx+sy*sy+sz*sz);if(sl<=0.00001f)sl=1.0f;sx/=sl;sy/=sl;sz/=sl;float vx=sy*fz-sz*fy,vy=sz*fx-sx*fz,vz=sx*fy-sy*fx;LpGuiMat4 m;_lp_m4_identity(&m);m.m[0]=sx;m.m[4]=sy;m.m[8]=sz;m.m[1]=vx;m.m[5]=vy;m.m[9]=vz;m.m[2]=-fx;m.m[6]=-fy;m.m[10]=-fz;m.m[12]=-(sx*ex+sy*ey+sz*ez);m.m[13]=-(vx*ex+vy*ey+vz*ez);m.m[14]=(fx*ex+fy*ey+fz*ez);return m;}
static LpGuiVertex _lp_gui_make_vertex(float x,float y,float z){_lp_m4_init();LpGuiMat4 mv=_lp_m4_mul(_lp_gui_view,_lp_gui_model);LpGuiMat4 mvp=_lp_m4_mul(_lp_gui_proj,mv);LpGuiVertex v;v.x=mvp.m[0]*x+mvp.m[4]*y+mvp.m[8]*z+mvp.m[12];v.y=mvp.m[1]*x+mvp.m[5]*y+mvp.m[9]*z+mvp.m[13];v.z=mvp.m[2]*x+mvp.m[6]*y+mvp.m[10]*z+mvp.m[14];v.w=mvp.m[3]*x+mvp.m[7]*y+mvp.m[11]*z+mvp.m[15];v.r=_lp_gui_cr;v.g=_lp_gui_cg;v.b=_lp_gui_cb;v.a=_lp_gui_ca;v.nx=_lp_gui_nx;v.ny=_lp_gui_ny;v.nz=_lp_gui_nz;v.nw=0;v.u=_lp_gui_tu;v.v=_lp_gui_tv;v._p0=v._p1=0;return v;}
static void _lp_gui_batch_add(LpGuiVertex v){if(_lp_gui_batch_n<(int)(sizeof(_lp_gui_batch)/sizeof(_lp_gui_batch[0])))_lp_gui_batch[_lp_gui_batch_n++]=v;}
static void _lp_gui_emit_vertex(float x,float y,float z){
    LpGuiVertex v;
    if(_lp_gui_backend_active==LP_GUI_BACKEND_DIRECTX11||_lp_gui_backend_active==LP_GUI_BACKEND_VULKAN){
        v.x=x;v.y=y;v.z=z;v.w=1.0f; /* DX11/Vulkan: GPU MVP via constant/push buffer */
    }else{
        v=_lp_gui_make_vertex(x,y,z); /* OpenGL: CPU MVP */
    }
    v.r=_lp_gui_cr;v.g=_lp_gui_cg;v.b=_lp_gui_cb;v.a=_lp_gui_ca;
    v.nx=_lp_gui_nx;v.ny=_lp_gui_ny;v.nz=_lp_gui_nz;v.nw=0;
    v.u=_lp_gui_tu;v.v=_lp_gui_tv;v._p0=v._p1=0;
    /* Vulkan GPU: Lambert lighting handled by SPIR-V shader via push constants */
    if(_lp_gui_mode==1){_lp_gui_quad[_lp_gui_quad_n++]=v;if(_lp_gui_quad_n==4){_lp_gui_batch_add(_lp_gui_quad[0]);_lp_gui_batch_add(_lp_gui_quad[1]);_lp_gui_batch_add(_lp_gui_quad[2]);_lp_gui_batch_add(_lp_gui_quad[0]);_lp_gui_batch_add(_lp_gui_quad[2]);_lp_gui_batch_add(_lp_gui_quad[3]);_lp_gui_quad_n=0;}}else _lp_gui_batch_add(v);
}
static inline const char*_lp_gui_vendor_name(UINT id){
    if(id==0x10DE)return"NVIDIA";
    if(id==0x1002||id==0x1022)return"AMD";
    if(id==0x8086)return"Intel";
    if(id==0x1414)return"Microsoft";
    return"Unknown";
}
static inline int _lp_gui_probe_dxgi_adapter(char*name,size_t name_sz,char*vendor,size_t vendor_sz){
    IDXGIFactory*factory=NULL;IDXGIAdapter*adapter=NULL;UINT i=0;int found=0;
    if(FAILED(CreateDXGIFactory(&IID_IDXGIFactory,(void**)&factory))||!factory)return 0;
    while(IDXGIFactory_EnumAdapters(factory,i++,&adapter)!=DXGI_ERROR_NOT_FOUND){
        DXGI_ADAPTER_DESC desc;HRESULT hr=IDXGIAdapter_GetDesc(adapter,&desc);
        if(SUCCEEDED(hr)&&desc.VendorId!=0x1414){
            if(name&&name_sz>0)WideCharToMultiByte(CP_UTF8,0,desc.Description,-1,name,(int)name_sz,NULL,NULL);
            if(vendor&&vendor_sz>0){const char*v=_lp_gui_vendor_name(desc.VendorId);lstrcpynA(vendor,v,(int)vendor_sz);}
            found=1;IDXGIAdapter_Release(adapter);break;
        }
        if(adapter)IDXGIAdapter_Release(adapter);adapter=NULL;
    }
    IDXGIFactory_Release(factory);
    return found;
}
static inline int lp_gui_directx_available(void){
    return _lp_gui_probe_dxgi_adapter(_lp_gui_dx_name,sizeof(_lp_gui_dx_name),_lp_gui_dx_vendor,sizeof(_lp_gui_dx_vendor));
    }
static inline int lp_gui_vulkan_available(void){
    HMODULE vk=LoadLibraryA("vulkan-1.dll");
    if(!vk){lstrcpynA(_lp_gui_vk_info,"Vulkan loader unavailable",sizeof(_lp_gui_vk_info));return 0;}
    lstrcpynA(_lp_gui_vk_info,"Vulkan loader detected (vulkan-1.dll)",sizeof(_lp_gui_vk_info));
    FreeLibrary(vk);return 1;
}
static inline void lp_gui_set_backend(int backend){_lp_gui_backend_pref=backend;}
static inline int lp_gui_get_backend(void){return _lp_gui_backend_active;}
static inline const char*lp_gui_backend_name(void){
    if(_lp_gui_backend_active==LP_GUI_BACKEND_DIRECTX11)return"DirectX 11";
    if(_lp_gui_backend_active==LP_GUI_BACKEND_VULKAN)return"Vulkan";
    if(_lp_gui_backend_active==LP_GUI_BACKEND_OPENGL)return"OpenGL";
    return"Auto";
}
static inline const char*lp_gui_backend_info(void){
    static char info[512];
    const char*dx=lp_gui_directx_available()?"yes":"no";
    const char*vk=lp_gui_vulkan_available()?"yes":"no";
    snprintf(info,sizeof(info),"backend=%s directx=%s vulkan=%s adapter=%s vendor=%s",
        lp_gui_backend_name(),dx,vk,_lp_gui_dx_name[0]?_lp_gui_dx_name:"N/A",_lp_gui_dx_vendor);
    return info;
}
static inline int _lp_gui_dx_init_pipeline(LpCanvas3D*g){
    static const char*src=
        "cbuffer CB:register(b0){float4x4 mvp;float4 lPos;float4 lCol;float4 amb;int4 flags;};"
        "Texture2D tex:register(t0);SamplerState smp:register(s0);"
        "struct VSIn{float4 pos:POSITION;float4 col:COLOR;float4 nrm:NORMAL;float4 uv:TEXCOORD;};"
        "struct PSIn{float4 pos:SV_POSITION;float4 col:COLOR;float3 nrm:NORMAL;float2 uv:TEXCOORD;float3 wp:WPOS;};"
        "PSIn vs_main(VSIn v){PSIn o;o.pos=mul(mvp,v.pos);o.col=v.col;o.nrm=v.nrm.xyz;o.uv=v.uv.xy;o.wp=v.pos.xyz;return o;}"
        "float4 ps_main(PSIn p):SV_TARGET{"
        "float4 c=p.col;"
        "if(flags.y)c*=tex.Sample(smp,p.uv);"
        "if(flags.x){float3 L=normalize(lPos.xyz-p.wp);float d=saturate(dot(normalize(p.nrm),L));c.rgb=c.rgb*(amb.rgb+lCol.rgb*d);}"
        "return c;}";
    ID3D10Blob*vsb=NULL;ID3D10Blob*psb=NULL;ID3D10Blob*err=NULL;HRESULT hr;
    hr=D3DCompile(src,strlen(src),NULL,NULL,NULL,"vs_main","vs_4_0",0,0,&vsb,&err);if(FAILED(hr)){if(err)ID3D10Blob_Release(err);return 0;}
    hr=D3DCompile(src,strlen(src),NULL,NULL,NULL,"ps_main","ps_4_0",0,0,&psb,&err);if(FAILED(hr)){if(vsb)ID3D10Blob_Release(vsb);if(err)ID3D10Blob_Release(err);return 0;}
    hr=ID3D11Device_CreateVertexShader(g->dx_device,ID3D10Blob_GetBufferPointer(vsb),ID3D10Blob_GetBufferSize(vsb),NULL,&g->dx_vs);if(FAILED(hr))goto fail;
    hr=ID3D11Device_CreatePixelShader(g->dx_device,ID3D10Blob_GetBufferPointer(psb),ID3D10Blob_GetBufferSize(psb),NULL,&g->dx_ps);if(FAILED(hr))goto fail;
    D3D11_INPUT_ELEMENT_DESC il[4]={{"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0},{"NORMAL",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,32,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,48,D3D11_INPUT_PER_VERTEX_DATA,0}};
    hr=ID3D11Device_CreateInputLayout(g->dx_device,il,4,ID3D10Blob_GetBufferPointer(vsb),ID3D10Blob_GetBufferSize(vsb),&g->dx_layout);if(FAILED(hr))goto fail;
    D3D11_BUFFER_DESC bd;ZeroMemory(&bd,sizeof(bd));bd.ByteWidth=sizeof(_lp_gui_batch);bd.Usage=D3D11_USAGE_DYNAMIC;bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;bd.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
    hr=ID3D11Device_CreateBuffer(g->dx_device,&bd,NULL,&g->dx_vbuf);if(FAILED(hr))goto fail;
    ID3D10Blob_Release(vsb);ID3D10Blob_Release(psb);
    /* Create constant buffer */
    {D3D11_BUFFER_DESC cbd;ZeroMemory(&cbd,sizeof(cbd));cbd.ByteWidth=sizeof(LpDxCB);cbd.Usage=D3D11_USAGE_DYNAMIC;cbd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;cbd.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
    ID3D11Device_CreateBuffer(g->dx_device,&cbd,NULL,&_lp_gui_dx_cb);}
    /* Create sampler */
    {D3D11_SAMPLER_DESC sd;ZeroMemory(&sd,sizeof(sd));sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;sd.MaxLOD=D3D11_FLOAT32_MAX;
    ID3D11Device_CreateSamplerState(g->dx_device,&sd,&_lp_gui_dx_sampler);}
    /* Create rasterizer */
    {D3D11_RASTERIZER_DESC rd;ZeroMemory(&rd,sizeof(rd));rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_BACK;rd.FrontCounterClockwise=FALSE;rd.DepthClipEnable=TRUE;
    ID3D11Device_CreateRasterizerState(g->dx_device,&rd,&_lp_gui_dx_rast);}
    return 1;
fail:
    if(vsb)ID3D10Blob_Release(vsb);if(psb)ID3D10Blob_Release(psb);return 0;
}
static inline void _lp_gui_dx_draw_batch(void){
    LpCanvas3D*g=_lp_active3d;if(!g||g->backend!=LP_GUI_BACKEND_DIRECTX11||!g->dx_ctx||!g->dx_vbuf||_lp_gui_batch_n<=0)return;
    D3D11_MAPPED_SUBRESOURCE map;HRESULT hr=ID3D11DeviceContext_Map(g->dx_ctx,(ID3D11Resource*)g->dx_vbuf,0,D3D11_MAP_WRITE_DISCARD,0,&map);if(FAILED(hr))return;
    memcpy(map.pData,_lp_gui_batch,(size_t)_lp_gui_batch_n*sizeof(LpGuiVertex));ID3D11DeviceContext_Unmap(g->dx_ctx,(ID3D11Resource*)g->dx_vbuf,0);
    UINT stride=sizeof(LpGuiVertex),offset=0;D3D11_VIEWPORT vp;vp.TopLeftX=0;vp.TopLeftY=0;vp.Width=(FLOAT)g->w;vp.Height=(FLOAT)g->h;vp.MinDepth=0;vp.MaxDepth=1;
    ID3D11DeviceContext_RSSetViewports(g->dx_ctx,1,&vp);
    ID3D11DeviceContext_OMSetRenderTargets(g->dx_ctx,1,&g->dx_rtv,g->dx_dsv);
    ID3D11DeviceContext_IASetInputLayout(g->dx_ctx,g->dx_layout);
    ID3D11DeviceContext_IASetVertexBuffers(g->dx_ctx,0,1,&g->dx_vbuf,&stride,&offset);
    ID3D11DeviceContext_IASetPrimitiveTopology(g->dx_ctx,(D3D11_PRIMITIVE_TOPOLOGY)_lp_gui_topology);
    ID3D11DeviceContext_VSSetShader(g->dx_ctx,g->dx_vs,NULL,0);
    ID3D11DeviceContext_PSSetShader(g->dx_ctx,g->dx_ps,NULL,0);
    /* Upload constant buffer with MVP + lighting */
    if(_lp_gui_dx_cb){
        _lp_m4_init();LpGuiMat4 mv=_lp_m4_mul(_lp_gui_view,_lp_gui_model);LpGuiMat4 mvp=_lp_m4_mul(_lp_gui_proj,mv);
        D3D11_MAPPED_SUBRESOURCE cbmap;
        if(SUCCEEDED(ID3D11DeviceContext_Map(g->dx_ctx,(ID3D11Resource*)_lp_gui_dx_cb,0,D3D11_MAP_WRITE_DISCARD,0,&cbmap))){
            LpDxCB*cb=(LpDxCB*)cbmap.pData;
            for(int r=0;r<4;r++)for(int c=0;c<4;c++)cb->mvp[r*4+c]=mvp.m[c*4+r]; /* transpose for HLSL */
            cb->lightPos[0]=_lp_gui_lx;cb->lightPos[1]=_lp_gui_ly;cb->lightPos[2]=_lp_gui_lz;cb->lightPos[3]=1;
            cb->lightCol[0]=_lp_gui_lr;cb->lightCol[1]=_lp_gui_lg;cb->lightCol[2]=_lp_gui_lb;cb->lightCol[3]=1;
            cb->ambient[0]=cb->ambient[1]=cb->ambient[2]=_lp_gui_ambient;cb->ambient[3]=1;
            cb->flags[0]=_lp_gui_lighting;cb->flags[1]=_lp_gui_texturing;cb->flags[2]=cb->flags[3]=0;
            ID3D11DeviceContext_Unmap(g->dx_ctx,(ID3D11Resource*)_lp_gui_dx_cb,0);}
        ID3D11DeviceContext_VSSetConstantBuffers(g->dx_ctx,0,1,&_lp_gui_dx_cb);
        ID3D11DeviceContext_PSSetConstantBuffers(g->dx_ctx,0,1,&_lp_gui_dx_cb);}
    if(_lp_gui_dx_sampler)ID3D11DeviceContext_PSSetSamplers(g->dx_ctx,0,1,&_lp_gui_dx_sampler);
    if(_lp_gui_dx_rast)ID3D11DeviceContext_RSSetState(g->dx_ctx,_lp_gui_dx_rast);
    ID3D11DeviceContext_Draw(g->dx_ctx,(UINT)_lp_gui_batch_n,0);
}
static inline int _lp_gui_canvasdx11(int p,int x,int y,int w,int h){
    if(!_lp_hInst)_lp_hInst=GetModuleHandle(NULL);
    if(!_lp_gl_cls_reg){WNDCLASSA wc={0};wc.lpfnWndProc=_lp_gui_wndproc;wc.hInstance=_lp_hInst;
        wc.lpszClassName="LpGL";wc.style=CS_HREDRAW|CS_VREDRAW|CS_OWNDC;wc.hCursor=LoadCursor(NULL,IDC_ARROW);
        RegisterClassA(&wc);_lp_gl_cls_reg=1;}
    if(_lp_c3d_n>=LP_GUI_MAX)return 0;
    int ww=w>0?w:640,hh=h>0?h:480;
    HWND hw=CreateWindowExA(0,"LpGL","",WS_CHILD|WS_VISIBLE,x,y,ww,hh,(HWND)(intptr_t)p,NULL,_lp_hInst,NULL);
    DXGI_SWAP_CHAIN_DESC sd;ZeroMemory(&sd,sizeof(sd));
    sd.BufferCount=1;sd.BufferDesc.Width=ww;sd.BufferDesc.Height=hh;sd.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;sd.OutputWindow=hw;sd.SampleDesc.Count=1;sd.Windowed=TRUE;sd.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL fl,levels[]={D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0};
    LpCanvas3D*g=&_lp_c3d[_lp_c3d_n];ZeroMemory(g,sizeof(*g));g->hwnd=hw;g->w=ww;g->h=hh;g->backend=LP_GUI_BACKEND_DIRECTX11;
    HRESULT hr=D3D11CreateDeviceAndSwapChain(NULL,D3D_DRIVER_TYPE_HARDWARE,NULL,0,levels,3,D3D11_SDK_VERSION,&sd,&g->dx_swap,&g->dx_device,&fl,&g->dx_ctx);
    if(FAILED(hr)||!g->dx_swap||!g->dx_device||!g->dx_ctx){DestroyWindow(hw);ZeroMemory(g,sizeof(*g));return 0;}
    ID3D11Texture2D*bb=NULL;
    hr=IDXGISwapChain_GetBuffer(g->dx_swap,0,&IID_ID3D11Texture2D,(void**)&bb);
    if(FAILED(hr)||!bb){if(g->dx_ctx)ID3D11DeviceContext_Release(g->dx_ctx);if(g->dx_device)ID3D11Device_Release(g->dx_device);if(g->dx_swap)IDXGISwapChain_Release(g->dx_swap);DestroyWindow(hw);ZeroMemory(g,sizeof(*g));return 0;}
    hr=ID3D11Device_CreateRenderTargetView(g->dx_device,(ID3D11Resource*)bb,NULL,&g->dx_rtv);
    ID3D11Texture2D_Release(bb);
    if(FAILED(hr)||!g->dx_rtv){if(g->dx_ctx)ID3D11DeviceContext_Release(g->dx_ctx);if(g->dx_device)ID3D11Device_Release(g->dx_device);if(g->dx_swap)IDXGISwapChain_Release(g->dx_swap);DestroyWindow(hw);ZeroMemory(g,sizeof(*g));return 0;}
    /* Create depth-stencil buffer */
    {D3D11_TEXTURE2D_DESC dtd;ZeroMemory(&dtd,sizeof(dtd));dtd.Width=ww;dtd.Height=hh;dtd.MipLevels=1;dtd.ArraySize=1;
    dtd.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;dtd.SampleDesc.Count=1;dtd.Usage=D3D11_USAGE_DEFAULT;dtd.BindFlags=D3D11_BIND_DEPTH_STENCIL;
    ID3D11Texture2D*dtex=NULL;hr=ID3D11Device_CreateTexture2D(g->dx_device,&dtd,NULL,&dtex);
    if(SUCCEEDED(hr)&&dtex){ID3D11Device_CreateDepthStencilView(g->dx_device,(ID3D11Resource*)dtex,NULL,&g->dx_dsv);ID3D11Texture2D_Release(dtex);}
    D3D11_DEPTH_STENCIL_DESC dsd;ZeroMemory(&dsd,sizeof(dsd));dsd.DepthEnable=TRUE;dsd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;
    dsd.DepthFunc=D3D11_COMPARISON_LESS;ID3D11Device_CreateDepthStencilState(g->dx_device,&dsd,&g->dx_dss);
    if(g->dx_dss)ID3D11DeviceContext_OMSetDepthStencilState(g->dx_ctx,g->dx_dss,0);}
    if(!_lp_gui_dx_init_pipeline(g)){if(g->dx_dsv)ID3D11DepthStencilView_Release(g->dx_dsv);if(g->dx_rtv)ID3D11RenderTargetView_Release(g->dx_rtv);if(g->dx_ctx)ID3D11DeviceContext_Release(g->dx_ctx);if(g->dx_device)ID3D11Device_Release(g->dx_device);if(g->dx_swap)IDXGISwapChain_Release(g->dx_swap);DestroyWindow(hw);ZeroMemory(g,sizeof(*g));return 0;}
    ID3D11DeviceContext_OMSetRenderTargets(g->dx_ctx,1,&g->dx_rtv,g->dx_dsv);
    _lp_c3d_n++;_lp_gui_backend_active=LP_GUI_BACKEND_DIRECTX11;_lp_active3d=g;lp_gui_directx_available();_lp_m4_init();
    return(int)(intptr_t)hw;
}
/* WndProc */
static LRESULT CALLBACK _lp_gui_wndproc(HWND h,UINT m,WPARAM w,LPARAM l){
    switch(m){
    case WM_DESTROY:PostQuitMessage(0);return 0;
    case WM_TIMER:if(_lp_on_timer)_lp_on_timer();return 0;
    case WM_KEYDOWN:_lp_keys[w&0xFF]=1;if(_lp_on_kdown)_lp_on_kdown((int)w);return 0;
    case WM_KEYUP:_lp_keys[w&0xFF]=0;if(_lp_on_kup)_lp_on_kup((int)w);return 0;
    case WM_MOUSEMOVE:_lp_mx=LOWORD(l);_lp_my=HIWORD(l);if(_lp_on_mmove)_lp_on_mmove(_lp_mx,_lp_my,0);return 0;
    case WM_LBUTTONDOWN:if(_lp_on_mclick)_lp_on_mclick(LOWORD(l),HIWORD(l),1);return 0;
    case WM_RBUTTONDOWN:if(_lp_on_mclick)_lp_on_mclick(LOWORD(l),HIWORD(l),2);return 0;
    case WM_MBUTTONDOWN:if(_lp_on_mclick)_lp_on_mclick(LOWORD(l),HIWORD(l),3);return 0;
    case WM_PAINT:{LpCanvas2D*c=_lp_fc2d(h);if(c&&c->memDC){PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);BitBlt(dc,0,0,c->w,c->h,c->memDC,0,0,SRCCOPY);EndPaint(h,&ps);return 0;}break;}
    case WM_SIZE:{
        LpCanvas3D*g=_lp_fc3d(h);
        if(g&&g->backend==LP_GUI_BACKEND_OPENGL){g->w=LOWORD(l);g->h=HIWORD(l);wglMakeCurrent(g->hdc,g->hglrc);glViewport(0,0,g->w,g->h);}
        break;}
    }
    return DefWindowProcA(h,m,w,l);
}

/* Window */
static inline int lp_gui_window(const char*t,int w,int h){
    if(!_lp_hInst)_lp_hInst=GetModuleHandle(NULL);
    if(!_lp_cls_reg){WNDCLASSA wc={0};wc.lpfnWndProc=_lp_gui_wndproc;wc.hInstance=_lp_hInst;
        wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);wc.lpszClassName="LpGuiClass";
        wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.style=CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
        RegisterClassA(&wc);_lp_cls_reg=1;}
    int ww=w>0?w:640,hh=h>0?h:480;
    RECT rc={0,0,ww,hh};AdjustWindowRect(&rc,WS_OVERLAPPEDWINDOW,FALSE);
    HWND hw=CreateWindowExA(0,"LpGuiClass",t?t:"LP GUI",WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        CW_USEDEFAULT,CW_USEDEFAULT,rc.right-rc.left,rc.bottom-rc.top,NULL,NULL,_lp_hInst,NULL);
    return(int)(intptr_t)hw;
}
static inline void lp_gui_close(int h){DestroyWindow((HWND)(intptr_t)h);}
static inline void lp_gui_set_title(int h,const char*t){SetWindowTextA((HWND)(intptr_t)h,t?t:"");}
/* Callbacks */
static inline void lp_gui_on_timer(LpGuiCB cb){_lp_on_timer=cb;}
static inline void lp_gui_on_key_down(LpGuiKeyCB cb){_lp_on_kdown=cb;}
static inline void lp_gui_on_key_up(LpGuiKeyCB cb){_lp_on_kup=cb;}
static inline void lp_gui_on_mouse_move(LpGuiMouseCB cb){_lp_on_mmove=cb;}
static inline void lp_gui_on_mouse_click(LpGuiMouseCB cb){_lp_on_mclick=cb;}
/* Input polling */
static inline int lp_gui_get_key_state(int k){return _lp_keys[k&0xFF];}
static inline int lp_gui_get_mouse_x(void){return _lp_mx;}
static inline int lp_gui_get_mouse_y(void){return _lp_my;}
/* Timer */
static inline int lp_gui_set_timer(int h,int ms){return(int)SetTimer((HWND)(intptr_t)h,1,ms>0?ms:16,NULL);}
static inline int lp_gui_set_fps(int h,int fps){return lp_gui_set_timer(h,fps>0?1000/fps:16);}
/* Event loops */
static inline void lp_gui_run(int h){(void)h;MSG m;while(GetMessage(&m,NULL,0,0)){TranslateMessage(&m);DispatchMessage(&m);}}
static inline void lp_gui_run_game(int h){(void)h;MSG m;for(;;){while(PeekMessage(&m,NULL,0,0,PM_REMOVE)){if(m.message==WM_QUIT)return;TranslateMessage(&m);DispatchMessage(&m);}if(_lp_on_timer)_lp_on_timer();Sleep(1);}}

/* === Basic Widgets === */
static inline int lp_gui_label(int p,const char*t,int x,int y){
    return(int)(intptr_t)CreateWindowExA(0,"STATIC",t?t:"",WS_CHILD|WS_VISIBLE|SS_LEFT,x,y,300,20,(HWND)(intptr_t)p,NULL,_lp_hInst,NULL);}
static inline int lp_gui_button(int p,const char*t,int x,int y,int w,int h){
    return(int)(intptr_t)CreateWindowExA(0,"BUTTON",t?t:"Button",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,x,y,w>0?w:100,h>0?h:30,(HWND)(intptr_t)p,NULL,_lp_hInst,NULL);}
static inline int lp_gui_input(int p,int x,int y,int w,int h){
    return(int)(intptr_t)CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,x,y,w>0?w:200,h>0?h:25,(HWND)(intptr_t)p,NULL,_lp_hInst,NULL);}
static inline int lp_gui_textarea(int p,int x,int y,int w,int h){
    return(int)(intptr_t)CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","",WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL,x,y,w>0?w:300,h>0?h:200,(HWND)(intptr_t)p,NULL,_lp_hInst,NULL);}
static inline int lp_gui_checkbox(int p,const char*t,int x,int y){
    return(int)(intptr_t)CreateWindowExA(0,"BUTTON",t?t:"",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,x,y,200,20,(HWND)(intptr_t)p,NULL,_lp_hInst,NULL);}
static inline int lp_gui_radio(int p,const char*t,int x,int y){
    return(int)(intptr_t)CreateWindowExA(0,"BUTTON",t?t:"",WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,x,y,200,20,(HWND)(intptr_t)p,NULL,_lp_hInst,NULL);}
static inline int lp_gui_group(int p,const char*t,int x,int y,int w,int h){
    return(int)(intptr_t)CreateWindowExA(0,"BUTTON",t?t:"",WS_CHILD|WS_VISIBLE|BS_GROUPBOX,x,y,w>0?w:200,h>0?h:150,(HWND)(intptr_t)p,NULL,_lp_hInst,NULL);}
static inline int lp_gui_dropdown(int p,int x,int y,int w,int h){
    return(int)(intptr_t)CreateWindowExA(0,"COMBOBOX","",WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,x,y,w>0?w:150,h>0?h:200,(HWND)(intptr_t)p,NULL,_lp_hInst,NULL);}
static inline int lp_gui_listbox(int p,int x,int y,int w,int h){
    return(int)(intptr_t)CreateWindowExA(WS_EX_CLIENTEDGE,"LISTBOX","",WS_CHILD|WS_VISIBLE|LBS_NOTIFY|WS_VSCROLL,x,y,w>0?w:200,h>0?h:150,(HWND)(intptr_t)p,NULL,_lp_hInst,NULL);}
static inline int lp_gui_slider(int p,int x,int y,int w,int h,int mn,int mx){
    HWND hw=CreateWindowExA(0,TRACKBAR_CLASSA,"",WS_CHILD|WS_VISIBLE|TBS_HORZ,x,y,w>0?w:200,h>0?h:30,(HWND)(intptr_t)p,NULL,_lp_hInst,NULL);
    SendMessage(hw,TBM_SETRANGE,TRUE,MAKELONG(mn,mx));return(int)(intptr_t)hw;}
static inline int lp_gui_progress(int p,int x,int y,int w,int h){
    return(int)(intptr_t)CreateWindowExA(0,PROGRESS_CLASSA,"",WS_CHILD|WS_VISIBLE,x,y,w>0?w:200,h>0?h:20,(HWND)(intptr_t)p,NULL,_lp_hInst,NULL);}
static inline int lp_gui_tab(int p,int x,int y,int w,int h){
    return(int)(intptr_t)CreateWindowExA(0,WC_TABCONTROLA,"",WS_CHILD|WS_VISIBLE,x,y,w>0?w:400,h>0?h:300,(HWND)(intptr_t)p,NULL,_lp_hInst,NULL);}
static inline int lp_gui_image(int p,const char*path,int x,int y,int w,int h){
    HWND hw=CreateWindowExA(0,"STATIC","",WS_CHILD|WS_VISIBLE|SS_BITMAP,x,y,w>0?w:100,h>0?h:100,(HWND)(intptr_t)p,NULL,_lp_hInst,NULL);
    if(path){HBITMAP bm=(HBITMAP)LoadImageA(NULL,path,IMAGE_BITMAP,w,h,LR_LOADFROMFILE);if(bm)SendMessage(hw,STM_SETIMAGE,IMAGE_BITMAP,(LPARAM)bm);}
    return(int)(intptr_t)hw;}

/* === Widget Operations === */
static inline void lp_gui_set_text(int h,const char*t){SetWindowTextA((HWND)(intptr_t)h,t?t:"");}
static inline const char*lp_gui_get_text(int h){int n=GetWindowTextLengthA((HWND)(intptr_t)h);char*b=(char*)malloc(n+2);if(b){GetWindowTextA((HWND)(intptr_t)h,b,n+1);b[n]=0;}return b?b:_strdup("");}
static inline void lp_gui_set_value(int h,int v){
    char cn[64];GetClassNameA((HWND)(intptr_t)h,cn,sizeof(cn));
    if(strcmp(cn,TRACKBAR_CLASSA)==0)SendMessage((HWND)(intptr_t)h,TBM_SETPOS,TRUE,v);
    else if(strcmp(cn,PROGRESS_CLASSA)==0)SendMessage((HWND)(intptr_t)h,PBM_SETPOS,v,0);}
static inline int lp_gui_get_value(int h){
    char cn[64];GetClassNameA((HWND)(intptr_t)h,cn,sizeof(cn));
    if(strcmp(cn,TRACKBAR_CLASSA)==0)return(int)SendMessage((HWND)(intptr_t)h,TBM_GETPOS,0,0);
    if(strcmp(cn,PROGRESS_CLASSA)==0)return(int)SendMessage((HWND)(intptr_t)h,PBM_GETPOS,0,0);
    return(int)SendMessage((HWND)(intptr_t)h,BM_GETCHECK,0,0);}
static inline void lp_gui_add_item(int h,const char*t){
    char cn[64];GetClassNameA((HWND)(intptr_t)h,cn,sizeof(cn));
    if(strcmp(cn,"ComboBox")==0)SendMessageA((HWND)(intptr_t)h,CB_ADDSTRING,0,(LPARAM)(t?t:""));
    else if(strcmp(cn,"ListBox")==0)SendMessageA((HWND)(intptr_t)h,LB_ADDSTRING,0,(LPARAM)(t?t:""));}
static inline void lp_gui_set_visible(int h,int v){ShowWindow((HWND)(intptr_t)h,v?SW_SHOW:SW_HIDE);}
static inline void lp_gui_set_enabled(int h,int e){EnableWindow((HWND)(intptr_t)h,e);}
static inline void lp_gui_set_font(int h,const char*name,int sz){
    HFONT f=CreateFontA(-sz,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,name?name:"Segoe UI");
    SendMessage((HWND)(intptr_t)h,WM_SETFONT,(WPARAM)f,TRUE);}
static inline void lp_gui_set_bg_color(int h,int color){(void)h;(void)color;/* requires subclassing - stub */}
/* Dialogs */
static inline void lp_gui_message_box(const char*t,const char*m){MessageBoxA(NULL,m?m:"",t?t:"LP",MB_OK|MB_ICONINFORMATION);}
static inline int lp_gui_confirm(const char*t,const char*m){return MessageBoxA(NULL,m?m:"",t?t:"LP",MB_YESNO|MB_ICONQUESTION)==IDYES;}
static inline const char*lp_gui_input_dialog(const char*t,const char*m){
    (void)t;(void)m;/* Simple implementation using a temp window */
    static char buf[512]={0};buf[0]=0;
    /* Fallback: prompt in console */
    printf("%s: %s\n> ",t?t:"Input",m?m:"");fgets(buf,sizeof(buf),stdin);
    int len=(int)strlen(buf);if(len>0&&buf[len-1]=='\n')buf[len-1]=0;return buf;}
static inline const char*lp_gui_file_open_dialog(const char*filter){
    static char fn[MAX_PATH]="";OPENFILENAMEA ofn={0};ofn.lStructSize=sizeof(ofn);
    ofn.lpstrFilter=filter?filter:"All\0*.*\0";ofn.lpstrFile=fn;ofn.nMaxFile=MAX_PATH;
    ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
    return GetOpenFileNameA(&ofn)?fn:"";}
static inline const char*lp_gui_file_save_dialog(const char*filter){
    static char fn[MAX_PATH]="";OPENFILENAMEA ofn={0};ofn.lStructSize=sizeof(ofn);
    ofn.lpstrFilter=filter?filter:"All\0*.*\0";ofn.lpstrFile=fn;ofn.nMaxFile=MAX_PATH;
    ofn.Flags=OFN_OVERWRITEPROMPT;
    return GetSaveFileNameA(&ofn)?fn:"";}
static inline const char*lp_gui_color_picker(void){
    static char buf[16];COLORREF c=0;COLORREF cc[16]={0};CHOOSECOLORA ch={0};
    ch.lStructSize=sizeof(ch);ch.rgbResult=c;ch.lpCustColors=cc;ch.Flags=CC_FULLOPEN|CC_RGBINIT;
    if(ChooseColorA(&ch)){snprintf(buf,sizeof(buf),"%d",ch.rgbResult);return buf;}return"";}
static inline const char*lp_gui_folder_dialog(void){
    static char path[MAX_PATH]="";BROWSEINFOA bi={0};bi.lpszTitle="Select Folder";
    LPITEMIDLIST pidl=SHBrowseForFolderA(&bi);
    if(pidl){SHGetPathFromIDListA(pidl,path);CoTaskMemFree(pidl);return path;}return"";}

/* ══════════ 2D Canvas ══════════ */
static inline int lp_gui_canvas(int p,int x,int y,int w,int h){
    HWND hw=CreateWindowExA(0,"STATIC","",WS_CHILD|WS_VISIBLE|SS_OWNERDRAW,x,y,w>0?w:400,h>0?h:300,(HWND)(intptr_t)p,NULL,_lp_hInst,NULL);
    if(_lp_c2d_n<LP_GUI_MAX){
        LpCanvas2D*c=&_lp_c2d[_lp_c2d_n++];c->hwnd=hw;c->w=w>0?w:400;c->h=h>0?h:300;
        HDC dc=GetDC(hw);c->memDC=CreateCompatibleDC(dc);c->memBmp=CreateCompatibleBitmap(dc,c->w,c->h);
        c->oldBmp=(HBITMAP)SelectObject(c->memDC,c->memBmp);ReleaseDC(hw,dc);
        /* Clear to black */
        RECT r={0,0,c->w,c->h};HBRUSH br=CreateSolidBrush(RGB(0,0,0));FillRect(c->memDC,&r,br);DeleteObject(br);
    }
    return(int)(intptr_t)hw;}
static inline LpCanvas2D*_lp_c2d_from(int h){return _lp_fc2d((HWND)(intptr_t)h);}
/* 2D Drawing */
static inline void lp_gui_clear(int cv,int color){
    LpCanvas2D*c=_lp_c2d_from(cv);if(!c)return;
    RECT r={0,0,c->w,c->h};HBRUSH br=CreateSolidBrush((COLORREF)color);FillRect(c->memDC,&r,br);DeleteObject(br);}
static inline void lp_gui_present(int cv){
    LpCanvas2D*c=_lp_c2d_from(cv);if(!c)return;
    HDC dc=GetDC(c->hwnd);BitBlt(dc,0,0,c->w,c->h,c->memDC,0,0,SRCCOPY);ReleaseDC(c->hwnd,dc);}
static inline void lp_gui_draw_rect(int cv,int x,int y,int w,int h,int color){
    LpCanvas2D*c=_lp_c2d_from(cv);if(!c)return;
    RECT r={x,y,x+w,y+h};HBRUSH br=CreateSolidBrush((COLORREF)color);FillRect(c->memDC,&r,br);DeleteObject(br);}
static inline void lp_gui_draw_rect_outline(int cv,int x,int y,int w,int h,int color,int th){
    LpCanvas2D*c=_lp_c2d_from(cv);if(!c)return;
    HPEN pen=CreatePen(PS_SOLID,th,(COLORREF)color);HPEN op=(HPEN)SelectObject(c->memDC,pen);
    HBRUSH ob=(HBRUSH)SelectObject(c->memDC,GetStockObject(NULL_BRUSH));
    Rectangle(c->memDC,x,y,x+w,y+h);SelectObject(c->memDC,op);SelectObject(c->memDC,ob);DeleteObject(pen);}
static inline void lp_gui_draw_circle(int cv,int cx,int cy,int r,int color){
    LpCanvas2D*c=_lp_c2d_from(cv);if(!c)return;
    HBRUSH br=CreateSolidBrush((COLORREF)color);HBRUSH ob=(HBRUSH)SelectObject(c->memDC,br);
    HPEN op=(HPEN)SelectObject(c->memDC,GetStockObject(NULL_PEN));
    Ellipse(c->memDC,cx-r,cy-r,cx+r,cy+r);SelectObject(c->memDC,ob);SelectObject(c->memDC,op);DeleteObject(br);}
static inline void lp_gui_draw_circle_outline(int cv,int cx,int cy,int r,int color,int th){
    LpCanvas2D*c=_lp_c2d_from(cv);if(!c)return;
    HPEN pen=CreatePen(PS_SOLID,th,(COLORREF)color);HPEN op=(HPEN)SelectObject(c->memDC,pen);
    HBRUSH ob=(HBRUSH)SelectObject(c->memDC,GetStockObject(NULL_BRUSH));
    Ellipse(c->memDC,cx-r,cy-r,cx+r,cy+r);SelectObject(c->memDC,op);SelectObject(c->memDC,ob);DeleteObject(pen);}
static inline void lp_gui_draw_line(int cv,int x1,int y1,int x2,int y2,int color,int th){
    LpCanvas2D*c=_lp_c2d_from(cv);if(!c)return;
    HPEN pen=CreatePen(PS_SOLID,th,(COLORREF)color);HPEN op=(HPEN)SelectObject(c->memDC,pen);
    MoveToEx(c->memDC,x1,y1,NULL);LineTo(c->memDC,x2,y2);SelectObject(c->memDC,op);DeleteObject(pen);}
static inline void lp_gui_draw_text(int cv,const char*t,int x,int y,int color,int sz){
    LpCanvas2D*c=_lp_c2d_from(cv);if(!c)return;
    HFONT f=CreateFontA(-sz,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,"Segoe UI");
    HFONT of=(HFONT)SelectObject(c->memDC,f);SetTextColor(c->memDC,(COLORREF)color);SetBkMode(c->memDC,TRANSPARENT);
    TextOutA(c->memDC,x,y,t?t:"",t?(int)strlen(t):0);SelectObject(c->memDC,of);DeleteObject(f);}
static inline void lp_gui_draw_pixel(int cv,int x,int y,int color){
    LpCanvas2D*c=_lp_c2d_from(cv);if(!c)return;SetPixel(c->memDC,x,y,(COLORREF)color);}
static inline void lp_gui_draw_triangle(int cv,int x1,int y1,int x2,int y2,int x3,int y3,int color){
    LpCanvas2D*c=_lp_c2d_from(cv);if(!c)return;
    POINT pts[3]={{x1,y1},{x2,y2},{x3,y3}};
    HBRUSH br=CreateSolidBrush((COLORREF)color);HBRUSH ob=(HBRUSH)SelectObject(c->memDC,br);
    HPEN op=(HPEN)SelectObject(c->memDC,GetStockObject(NULL_PEN));
    Polygon(c->memDC,pts,3);SelectObject(c->memDC,ob);SelectObject(c->memDC,op);DeleteObject(br);}
static inline void lp_gui_draw_image(int cv,const char*path,int x,int y,int w,int h){
    LpCanvas2D*c=_lp_c2d_from(cv);if(!c||!path)return;
    HBITMAP bm=(HBITMAP)LoadImageA(NULL,path,IMAGE_BITMAP,w,h,LR_LOADFROMFILE);
    if(!bm)return;HDC tmp=CreateCompatibleDC(c->memDC);HBITMAP ob=(HBITMAP)SelectObject(tmp,bm);
    BitBlt(c->memDC,x,y,w,h,tmp,0,0,SRCCOPY);SelectObject(tmp,ob);DeleteDC(tmp);DeleteObject(bm);}

/* ══════════ 3D Canvas (OpenGL) ══════════ */
/* ═══════════════════ Vulkan GPU Pipeline (Zero-SDK, Hex-String SPIR-V) ═══════════════════ */
/* SPIR-V shaders as hex strings — avoids GCC ICE on large static arrays */
static const char _lp_vk_vs_hex[]="03022307000001000b000d00300000000000000011000200010000000b00060001000000474c534c2e7374642e343530000000000e00030000000000010000000f000e0000000000040000006d61696e000000000d000000190000001e0000001f0000002300000024000000290000002a0000002d0000000300030002000000c201000004000a00474c5f474f4f474c455f6370705f7374796c655f6c696e655f646972656374697665000004000800474c5f474f4f474c455f696e636c7564655f6469726563746976650005000400040000006d61696e00000000050006000b000000676c5f50657256657274657800000000060006000b00000000000000676c5f506f736974696f6e00060007000b00000001000000676c5f506f696e7453697a6500000000060007000b00000002000000676c5f436c697044697374616e636500060007000b00000003000000676c5f43756c6c44697374616e636500050003000d000000000000000500030012000000504300000600040012000000000000006d7670000600060012000000010000006c69676874506f73000000000600060012000000020000006c69676874436f6c00000000060005001200000003000000616d6269656e7400060005001200000004000000666c6167730000000500030014000000706300000500040019000000696e506f73000000050004001e00000076436f6c00000000050004001f000000696e436f6c0000000500040023000000764e726d000000000500040024000000696e4e726d000000050003002900000076555600050004002a000000696e555600000000050004002d0000007657506f73000000470003000b00000002000000480005000b000000000000000b00000000000000480005000b000000010000000b00000001000000480005000b000000020000000b00000003000000480005000b000000030000000b000000040000004700030012000000020000004800040012000000000000000500000048000500120000000000000007000000100000004800050012000000000000002300000000000000480005001200000001000000230000004000000048000500120000000200000023000000500000004800050012000000030000002300000060000000480005001200000004000000230000007000000047000400190000001e00000000000000470004001e0000001e00000000000000470004001f0000001e0000000100000047000400230000001e0000000100000047000400240000001e0000000200000047000400290000001e00000002000000470004002a0000001e00000003000000470004002d0000001e00000003000000130002000200000021000300030000000200000016000300060000002000000017000400070000000600000004000000150004000800000020000000000000002b0004000800000009000000010000001c0004000a00000006000000090000001e0006000b00000007000000060000000a0000000a000000200004000c000000030000000b0000003b0004000c0000000d00000003000000150004000e00000020000000010000002b0004000e0000000f000000000000001800040010000000070000000400000017000400110000000e000000040000001e000700120000001000000007000000070000000700000011000000200004001300000009000000120000003b00040013000000140000000900000020000400150000000900000010000000200004001800000001000000070000003b000400180000001900000001000000200004001c00000003000000070000003b0004001c0000001e000000030000003b000400180000001f0000000100000017000400210000000600000003000000200004002200000003000000210000003b0004002200000023000000030000003b00040018000000240000000100000017000400270000000600000002000000200004002800000003000000270000003b0004002800000029000000030000003b000400180000002a000000010000003b000400220000002d000000030000003600050002000000040000000000000003000000f800020005000000410005001500000016000000140000000f0000003d0004001000000017000000160000003d000400070000001a0000001900000091000500070000001b000000170000001a000000410005001c0000001d0000000d0000000f0000003e0003001d0000001b0000003d00040007000000200000001f0000003e0003001e000000200000003d0004000700000025000000240000004f000800210000002600000025000000250000000000000001000000020000003e00030023000000260000003d000400070000002b0000002a0000004f000700270000002c0000002b0000002b00000000000000010000003e000300290000002c0000003d000400070000002e000000190000004f000800210000002f0000002e0000002e0000000000000001000000020000003e0003002d0000002f000000fd00010038000100";
static const char _lp_vk_fs_hex[]="03022307000001000b000d005e0000000000000011000200010000000b00060001000000474c534c2e7374642e343530000000000e00030000000000010000000f000a0004000000040000006d61696e000000000b00000025000000390000003f0000005c0000001000030004000000070000000300030002000000c201000004000a00474c5f474f4f474c455f6370705f7374796c655f6c696e655f646972656374697665000004000800474c5f474f4f474c455f696e636c7564655f6469726563746976650005000400040000006d61696e00000000050003000900000063000000050004000b00000076436f6c000000000500030010000000504300000600040010000000000000006d7670000600060010000000010000006c69676874506f73000000000600060010000000020000006c69676874436f6c00000000060005001000000003000000616d6269656e7400060005001000000004000000666c61677300000005000300120000007063000005000300210000007465780005000300250000007655560005000300320000004c00000005000400390000007657506f73000000050004003e0000006469666600000000050004003f000000764e726d00000000050005005c0000006f7574436f6c6f7200000000470004000b0000001e0000000000000047000300100000000200000048000400100000000000000005000000480005001000000000000000070000001000000048000500100000000000000023000000000000004800050010000000010000002300000040000000480005001000000002000000230000005000000048000500100000000300000023000000600000004800050010000000040000002300000070000000470004002100000021000000000000004700040021000000220000000000000047000400250000001e0000000200000047000400390000001e00000003000000470004003f0000001e00000001000000470004005c0000001e0000000000000013000200020000002100030003000000020000001600030006000000200000001700040007000000060000000400000020000400080000000700000007000000200004000a00000001000000070000003b0004000a0000000b00000001000000180004000d0000000700000004000000150004000e0000002000000001000000170004000f0000000e000000040000001e000700100000000d0000000700000007000000070000000f000000200004001100000009000000100000003b0004001100000012000000090000002b0004000e0000001300000004000000150004001400000020000000000000002b0004001400000015000000010000002000040016000000090000000e0000002b0004000e0000001900000000000000140002001a000000190009001e000000060000000100000000000000000000000000000001000000000000001b0003001f0000001e0000002000040020000000000000001f0000003b00040020000000210000000000000017000400230000000600000002000000200004002400000001000000230000003b0004002400000025000000010000002b000400140000002a0000000000000017000400300000000600000003000000200004003100000007000000300000002b0004000e000000330000000100000020000400340000000900000007000000200004003800000001000000300000003b000400380000003900000001000000200004003d00000007000000060000003b000400380000003f000000010000002b0004000600000044000000000000002b0004000e00000048000000030000002b0004000e0000004c000000020000002b000400140000005800000002000000200004005b00000003000000070000003b0004005b0000005c000000030000003600050002000000040000000000000003000000f8000200050000003b0004000800000009000000070000003b0004003100000032000000070000003b0004003d0000003e000000070000003d000400070000000c0000000b0000003e000300090000000c0000004100060016000000170000001200000013000000150000003d0004000e0000001800000017000000ab0005001a0000001b0000001800000019000000f70003001d00000000000000fa0004001b0000001c0000001d000000f80002001c0000003d0004001f00000022000000210000003d00040023000000260000002500000057000500070000002700000022000000260000003d00040007000000280000000900000085000500070000002900000028000000270000003e0003000900000029000000f90002001d000000f80002001d00000041000600160000002b00000012000000130000002a0000003d0004000e0000002c0000002b000000ab0005001a0000002d0000002c00000019000000f70003002f00000000000000fa0004002d0000002e0000002f000000f80002002e00000041000500340000003500000012000000330000003d0004000700000036000000350000004f000800300000003700000036000000360000000000000001000000020000003d000400300000003a0000003900000083000500300000003b000000370000003a0000000c000600300000003c00000001000000450000003b0000003e000300320000003c0000003d00040030000000400000003f0000000c00060030000000410000000100000045000000400000003d00040030000000420000003200000094000500060000004300000041000000420000000c0007000600000045000000010000002800000043000000440000003e0003003e000000450000003d0004000700000046000000090000004f0008003000000047000000460000004600000000000000010000000200000041000500340000004900000012000000480000003d000400070000004a000000490000004f000800300000004b0000004a0000004a00000000000000010000000200000041000500340000004d000000120000004c0000003d000400070000004e0000004d0000004f000800300000004f0000004e0000004e0000000000000001000000020000003d00040006000000500000003e0000008e00050030000000510000004f000000500000008100050030000000520000004b000000510000008500050030000000530000004700000052000000410005003d00000054000000090000002a00000051000500060000005500000053000000000000003e0003005400000055000000410005003d00000056000000090000001500000051000500060000005700000053000000010000003e0003005600000057000000410005003d00000059000000090000005800000051000500060000005a00000053000000020000003e000300590000005a000000f90002002f000000f80002002f0000003d000400070000005d000000090000003e0003005c0000005d000000fd00010038000100";

static unsigned char*_lp_vk_hex_decode(const char*hex,int*out_len){
    int slen=(int)strlen(hex);*out_len=slen/2;
    unsigned char*buf=(unsigned char*)malloc(*out_len);
    for(int i=0;i<*out_len;i++){
        unsigned int hi,lo;
        hi=(hex[i*2]>='a')?hex[i*2]-'a'+10:hex[i*2]-'0';
        lo=(hex[i*2+1]>='a')?hex[i*2+1]-'a'+10:hex[i*2+1]-'0';
        buf[i]=(unsigned char)((hi<<4)|lo);
    }
    return buf;
}

/* Vulkan inline types (zero-SDK) */
#define VK_NULL_HANDLE 0
typedef uint64_t VkDeviceSize;
typedef uint32_t VkFlags,VkBool32;
typedef void*VkInstance,*VkPhysicalDevice,*VkDevice,*VkQueue,*VkCommandPool,*VkCommandBuffer;
typedef void*VkRenderPass,*VkPipeline,*VkPipelineLayout,*VkShaderModule,*VkDescriptorSetLayout;
typedef uint64_t VkSurfaceKHR,VkSwapchainKHR,VkImage,VkImageView,VkFramebuffer;
typedef uint64_t VkSemaphore,VkFence,VkDeviceMemory,VkBuffer,VkSampler,VkDescriptorPool,VkDescriptorSet;
#define VK_SUCCESS 0
#define VK_STRUCTURE_TYPE_APPLICATION_INFO 0
#define VK_STRUCTURE_TYPE_INSTANCE_CREATE 1
#define VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE 2
#define VK_STRUCTURE_TYPE_DEVICE_CREATE 3
#define VK_STRUCTURE_TYPE_SUBMIT_INFO 4
#define VK_STRUCTURE_TYPE_FENCE_CREATE 8
#define VK_STRUCTURE_TYPE_SEMAPHORE_CREATE 9
#define VK_STRUCTURE_TYPE_IMAGE_CREATE 14
#define VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE 15
#define VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE 16
#define VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE 28
#define VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE 30
#define VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE 19
#define VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE 20
#define VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE 22
#define VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE 23
#define VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE 24
#define VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE 26
#define VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE 27
#define VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE 35
#define VK_STRUCTURE_TYPE_RENDER_PASS_CREATE 36
#define VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE 37
#define VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE 38
#define VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN 39
#define VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN 40
#define VK_STRUCTURE_TYPE_BUFFER_CREATE 12
#define VK_STRUCTURE_TYPE_MEMORY_ALLOCATE 5
#define VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE 1000009000
#define VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE 1000001000
#define VK_STRUCTURE_TYPE_PRESENT_INFO 1000001001
#define VK_FORMAT_B8G8R8A8_UNORM 44
#define VK_FORMAT_D32_SFLOAT 126
#define VK_FORMAT_R32G32B32A32_SFLOAT 109
#define VK_IMAGE_LAYOUT_UNDEFINED 0
#define VK_IMAGE_LAYOUT_PRESENT_SRC 1000001002
#define VK_IMAGE_LAYOUT_COLOR_ATTACHMENT 2
#define VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT 3
#define VK_IMAGE_USAGE_COLOR_ATTACHMENT 0x10
#define VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT 0x20
#define VK_SAMPLE_COUNT_1 1
#define VK_ATTACHMENT_LOAD_OP_CLEAR 1
#define VK_ATTACHMENT_STORE_OP_STORE 0
#define VK_ATTACHMENT_STORE_OP_DONT_CARE 2
#define VK_PIPELINE_BIND_POINT_GRAPHICS 0
#define VK_SUBPASS_CONTENTS_INLINE 0
#define VK_SHADER_STAGE_VERTEX 1
#define VK_SHADER_STAGE_FRAGMENT 0x10
#define VK_COLOR_COMPONENT_ALL 0xF
#define VK_DYNAMIC_STATE_VIEWPORT 0
#define VK_DYNAMIC_STATE_SCISSOR 1
#define VK_COMMAND_BUFFER_LEVEL_PRIMARY 0
#define VK_COMMAND_BUFFER_USAGE_ONE_TIME 1
#define VK_FENCE_CREATE_SIGNALED 1
#define VK_COMMAND_POOL_CREATE_RESET 2
#define VK_COMPARE_OP_LESS 1
#define VK_IMAGE_TYPE_2D 1
#define VK_IMAGE_TILING_OPTIMAL 0
#define VK_IMAGE_ASPECT_DEPTH 2
#define VK_IMAGE_VIEW_TYPE_2D 1
#define VK_MEMORY_PROPERTY_DEVICE_LOCAL 1
#define VK_BUFFER_USAGE_VERTEX_BUFFER 0x80
#define VK_MEMORY_PROPERTY_HOST_VISIBLE 2
#define VK_MEMORY_PROPERTY_HOST_COHERENT 4
#ifndef VKAPI_CALL
#define VKAPI_CALL __stdcall
#endif

typedef struct{
    HMODULE vklib;VkInstance inst;VkPhysicalDevice pdev;VkDevice dev;VkQueue queue;uint32_t qfi;
    VkSurfaceKHR surface;VkSwapchainKHR swapchain;
    VkImage images[4];VkImageView views[4];uint32_t imageCount;
    VkRenderPass renderPass;VkFramebuffer fbs[4];
    VkPipelineLayout playout;VkPipeline pipeline;
    VkCommandPool cmdPool;VkCommandBuffer cmdBuf;
    VkSemaphore semAvail,semDone;VkFence fence;
    VkImage depthImage;VkDeviceMemory depthMem;VkImageView depthView;
    VkBuffer vbuf;VkDeviceMemory vbufMem;void*vbufMap;uint32_t vbufSize;
}_LpVkCtx;
static _LpVkCtx _lp_vk_ctx[4];static int _lp_vk_n=0;static _LpVkCtx*_lp_vk_active=NULL;
static float _lp_vk_clear_r=0,_lp_vk_clear_g=0,_lp_vk_clear_b=0;
#define LP_VK_MAX 4

/* Dynamic function pointers */
#define LP_VK_FN(ret,name,...) typedef ret(VKAPI_CALL*PFN_##name)(__VA_ARGS__);static PFN_##name p_##name=NULL;
LP_VK_FN(int32_t,vkCreateInstance,const void*,const void*,VkInstance*)
LP_VK_FN(void,vkDestroyInstance,VkInstance,const void*)
LP_VK_FN(int32_t,vkEnumeratePhysicalDevices,VkInstance,uint32_t*,VkPhysicalDevice*)
LP_VK_FN(void,vkGetPhysicalDeviceQueueFamilyProperties,VkPhysicalDevice,uint32_t*,void*)
LP_VK_FN(int32_t,vkCreateDevice,VkPhysicalDevice,const void*,const void*,VkDevice*)
LP_VK_FN(void,vkDestroyDevice,VkDevice,const void*)
LP_VK_FN(void,vkGetDeviceQueue,VkDevice,uint32_t,uint32_t,VkQueue*)
LP_VK_FN(int32_t,vkCreateWin32SurfaceKHR,VkInstance,const void*,const void*,VkSurfaceKHR*)
LP_VK_FN(int32_t,vkCreateSwapchainKHR,VkDevice,const void*,const void*,VkSwapchainKHR*)
LP_VK_FN(int32_t,vkGetSwapchainImagesKHR,VkDevice,VkSwapchainKHR,uint32_t*,VkImage*)
LP_VK_FN(int32_t,vkCreateImageView,VkDevice,const void*,const void*,VkImageView*)
LP_VK_FN(int32_t,vkCreateRenderPass,VkDevice,const void*,const void*,VkRenderPass*)
LP_VK_FN(int32_t,vkCreateFramebuffer,VkDevice,const void*,const void*,VkFramebuffer*)
LP_VK_FN(int32_t,vkCreateShaderModule,VkDevice,const void*,const void*,VkShaderModule*)
LP_VK_FN(void,vkDestroyShaderModule,VkDevice,VkShaderModule,const void*)
LP_VK_FN(int32_t,vkCreatePipelineLayout,VkDevice,const void*,const void*,VkPipelineLayout*)
LP_VK_FN(int32_t,vkCreateGraphicsPipelines,VkDevice,uint64_t,uint32_t,const void*,const void*,VkPipeline*)
LP_VK_FN(int32_t,vkCreateCommandPool,VkDevice,const void*,const void*,VkCommandPool*)
LP_VK_FN(int32_t,vkAllocateCommandBuffers,VkDevice,const void*,VkCommandBuffer*)
LP_VK_FN(int32_t,vkCreateSemaphore,VkDevice,const void*,const void*,VkSemaphore*)
LP_VK_FN(int32_t,vkCreateFence,VkDevice,const void*,const void*,VkFence*)
LP_VK_FN(int32_t,vkWaitForFences,VkDevice,uint32_t,const VkFence*,VkBool32,uint64_t)
LP_VK_FN(int32_t,vkResetFences,VkDevice,uint32_t,const VkFence*)
LP_VK_FN(int32_t,vkAcquireNextImageKHR,VkDevice,VkSwapchainKHR,uint64_t,VkSemaphore,VkFence,uint32_t*)
LP_VK_FN(int32_t,vkResetCommandBuffer,VkCommandBuffer,uint32_t)
LP_VK_FN(int32_t,vkBeginCommandBuffer,VkCommandBuffer,const void*)
LP_VK_FN(void,vkCmdBeginRenderPass,VkCommandBuffer,const void*,uint32_t)
LP_VK_FN(void,vkCmdEndRenderPass,VkCommandBuffer)
LP_VK_FN(void,vkCmdBindPipeline,VkCommandBuffer,uint32_t,VkPipeline)
LP_VK_FN(void,vkCmdSetViewport,VkCommandBuffer,uint32_t,uint32_t,const void*)
LP_VK_FN(void,vkCmdSetScissor,VkCommandBuffer,uint32_t,uint32_t,const void*)
LP_VK_FN(void,vkCmdDraw,VkCommandBuffer,uint32_t,uint32_t,uint32_t,uint32_t)
LP_VK_FN(int32_t,vkEndCommandBuffer,VkCommandBuffer)
LP_VK_FN(int32_t,vkQueueSubmit,VkQueue,uint32_t,const void*,VkFence)
LP_VK_FN(int32_t,vkQueuePresentKHR,VkQueue,const void*)
LP_VK_FN(void,vkCmdBindVertexBuffers,VkCommandBuffer,uint32_t,uint32_t,const VkBuffer*,const VkDeviceSize*)
LP_VK_FN(void,vkCmdPushConstants,VkCommandBuffer,VkPipelineLayout,uint32_t,uint32_t,uint32_t,const void*)
LP_VK_FN(int32_t,vkCreateImage,VkDevice,const void*,const void*,VkImage*)
LP_VK_FN(void,vkGetImageMemoryRequirements,VkDevice,VkImage,void*)
LP_VK_FN(int32_t,vkAllocateMemory,VkDevice,const void*,const void*,VkDeviceMemory*)
LP_VK_FN(int32_t,vkBindImageMemory,VkDevice,VkImage,VkDeviceMemory,VkDeviceSize)
LP_VK_FN(void,vkGetPhysicalDeviceMemoryProperties,VkPhysicalDevice,void*)
LP_VK_FN(int32_t,vkCreateBuffer,VkDevice,const void*,const void*,VkBuffer*)
LP_VK_FN(void,vkGetBufferMemoryRequirements,VkDevice,VkBuffer,void*)
LP_VK_FN(int32_t,vkBindBufferMemory,VkDevice,VkBuffer,VkDeviceMemory,VkDeviceSize)
LP_VK_FN(int32_t,vkMapMemory,VkDevice,VkDeviceMemory,VkDeviceSize,VkDeviceSize,uint32_t,void**)
LP_VK_FN(int32_t,vkDeviceWaitIdle,VkDevice)

static int _lp_vk_load_funcs(HMODULE lib){
    #define VK_LOAD(n) p_##n=(PFN_##n)GetProcAddress(lib,#n);if(!p_##n)return 0;
    VK_LOAD(vkCreateInstance)VK_LOAD(vkDestroyInstance)VK_LOAD(vkEnumeratePhysicalDevices)
    VK_LOAD(vkGetPhysicalDeviceQueueFamilyProperties)VK_LOAD(vkCreateDevice)VK_LOAD(vkDestroyDevice)
    VK_LOAD(vkGetDeviceQueue)VK_LOAD(vkCreateWin32SurfaceKHR)VK_LOAD(vkCreateSwapchainKHR)
    VK_LOAD(vkGetSwapchainImagesKHR)VK_LOAD(vkCreateImageView)VK_LOAD(vkCreateRenderPass)
    VK_LOAD(vkCreateFramebuffer)VK_LOAD(vkCreateShaderModule)VK_LOAD(vkDestroyShaderModule)
    VK_LOAD(vkCreatePipelineLayout)VK_LOAD(vkCreateGraphicsPipelines)VK_LOAD(vkCreateCommandPool)
    VK_LOAD(vkAllocateCommandBuffers)VK_LOAD(vkCreateSemaphore)VK_LOAD(vkCreateFence)
    VK_LOAD(vkWaitForFences)VK_LOAD(vkResetFences)VK_LOAD(vkAcquireNextImageKHR)
    VK_LOAD(vkResetCommandBuffer)VK_LOAD(vkBeginCommandBuffer)VK_LOAD(vkCmdBeginRenderPass)
    VK_LOAD(vkCmdEndRenderPass)VK_LOAD(vkCmdBindPipeline)VK_LOAD(vkCmdSetViewport)
    VK_LOAD(vkCmdSetScissor)VK_LOAD(vkCmdDraw)VK_LOAD(vkEndCommandBuffer)
    VK_LOAD(vkQueueSubmit)VK_LOAD(vkQueuePresentKHR)VK_LOAD(vkCmdBindVertexBuffers)
    VK_LOAD(vkCmdPushConstants)VK_LOAD(vkCreateImage)VK_LOAD(vkGetImageMemoryRequirements)
    VK_LOAD(vkAllocateMemory)VK_LOAD(vkBindImageMemory)VK_LOAD(vkGetPhysicalDeviceMemoryProperties)
    VK_LOAD(vkCreateBuffer)VK_LOAD(vkGetBufferMemoryRequirements)VK_LOAD(vkBindBufferMemory)
    VK_LOAD(vkMapMemory)VK_LOAD(vkDeviceWaitIdle)
    #undef VK_LOAD
    return 1;
}


static inline int lp_gui_canvas3d(int p,int x,int y,int w,int h){
    if(_lp_gui_backend_pref==LP_GUI_BACKEND_DIRECTX11 && lp_gui_directx_available()){
        int dxh=_lp_gui_canvasdx11(p,x,y,w,h);
        if(dxh)return dxh;
    }

    if(_lp_gui_backend_pref==LP_GUI_BACKEND_VULKAN&&_lp_vk_n<LP_VK_MAX){
        HMODULE vklib=LoadLibraryA("vulkan-1.dll");
        if(vklib&&_lp_vk_load_funcs(vklib)){
            if(!_lp_hInst)_lp_hInst=GetModuleHandle(NULL);
            if(!_lp_gl_cls_reg){WNDCLASSA wc={0};wc.lpfnWndProc=_lp_gui_wndproc;wc.hInstance=_lp_hInst;
                wc.lpszClassName="LpGL";wc.style=CS_HREDRAW|CS_VREDRAW|CS_OWNDC;wc.hCursor=LoadCursor(NULL,IDC_ARROW);
                RegisterClassA(&wc);_lp_gl_cls_reg=1;}
            int ww=w>0?w:640,hh=h>0?h:480;
            HWND hw=CreateWindowExA(0,"LpGL","",WS_CHILD|WS_VISIBLE,x,y,ww,hh,(HWND)(intptr_t)p,NULL,_lp_hInst,NULL);
            _LpVkCtx*vk=&_lp_vk_ctx[_lp_vk_n];ZeroMemory(vk,sizeof(*vk));vk->vklib=vklib;
            /* Instance */
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;const void*pAppInfo;uint32_t eLCount;const char*const*eLNames;uint32_t eEC;const char*const*eEN;}ICI;
            const char*exts[]={"VK_KHR_surface","VK_KHR_win32_surface"};
            ICI ici={VK_STRUCTURE_TYPE_INSTANCE_CREATE,NULL,0,NULL,0,NULL,2,exts};
            if(p_vkCreateInstance(&ici,NULL,&vk->inst)!=VK_SUCCESS){FreeLibrary(vklib);DestroyWindow(hw);goto vk_fail;}
            /* Physical device + queue family */
            uint32_t dc=0;p_vkEnumeratePhysicalDevices(vk->inst,&dc,NULL);
            if(dc==0)goto vk_fail_inst;
            VkPhysicalDevice pds[8];if(dc>8)dc=8;p_vkEnumeratePhysicalDevices(vk->inst,&dc,pds);vk->pdev=pds[0];
            uint32_t qfc=0;p_vkGetPhysicalDeviceQueueFamilyProperties(vk->pdev,&qfc,NULL);vk->qfi=0;
            /* Device */
            float qp=1.0f;
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;uint32_t qfi;uint32_t qCount;const float*pPri;}DQCI;
            DQCI dqci={VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE,NULL,0,vk->qfi,1,&qp};
            const char*dexts[]={"VK_KHR_swapchain"};
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;uint32_t qCICount;const DQCI*pQCI;uint32_t eLC;const char*const*eLN;uint32_t eDC;const char*const*eDN;const void*pEF;}DCI;
            DCI dci={VK_STRUCTURE_TYPE_DEVICE_CREATE,NULL,0,1,&dqci,0,NULL,1,dexts,NULL};
            if(p_vkCreateDevice(vk->pdev,&dci,NULL,&vk->dev)!=VK_SUCCESS)goto vk_fail_inst;
            p_vkGetDeviceQueue(vk->dev,vk->qfi,0,&vk->queue);
            /* Surface + Swapchain */
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;HINSTANCE hinstance;HWND hwnd;}WSCI;
            WSCI sci={VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE,NULL,0,_lp_hInst,hw};
            if(p_vkCreateWin32SurfaceKHR(vk->inst,&sci,NULL,&vk->surface)!=VK_SUCCESS)goto vk_fail_dev;
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;VkSurfaceKHR surface;uint32_t minIC;uint32_t format;uint32_t colorSpace;uint32_t ew,eh;uint32_t il;uint32_t layers;uint32_t usage;uint32_t sharingMode;uint32_t qfiC;const uint32_t*qfiI;uint32_t preTransform;uint32_t compositeAlpha;uint32_t presentMode;VkBool32 clipped;VkSwapchainKHR old;}SCCI;
            SCCI swci;ZeroMemory(&swci,sizeof(swci));swci.sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE;swci.surface=vk->surface;swci.minIC=2;swci.format=VK_FORMAT_B8G8R8A8_UNORM;swci.ew=(uint32_t)ww;swci.eh=(uint32_t)hh;swci.il=VK_IMAGE_LAYOUT_UNDEFINED;swci.layers=1;swci.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT;swci.compositeAlpha=1;swci.presentMode=1;swci.clipped=1;swci.preTransform=1;
            if(p_vkCreateSwapchainKHR(vk->dev,&swci,NULL,&vk->swapchain)!=VK_SUCCESS)goto vk_fail_dev;
            vk->imageCount=4;p_vkGetSwapchainImagesKHR(vk->dev,vk->swapchain,&vk->imageCount,vk->images);
            /* Image views */
            for(uint32_t i=0;i<vk->imageCount;i++){
                typedef struct{uint32_t sType;const void*pNext;uint32_t flags;VkImage image;uint32_t viewType;uint32_t format;uint32_t cr,cg,cb,ca;uint32_t aspectMask;uint32_t baseMip;uint32_t mipCount;uint32_t baseLayer;uint32_t layerCount;}IVCI;
                IVCI ivci={VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE,NULL,0,vk->images[i],VK_IMAGE_VIEW_TYPE_2D,VK_FORMAT_B8G8R8A8_UNORM,0,0,0,0,1,0,1,0,1};
                p_vkCreateImageView(vk->dev,&ivci,NULL,&vk->views[i]);}
            /* Depth image */
            {typedef struct{uint32_t sType;const void*pNext;uint32_t flags;uint32_t imageType;uint32_t format;uint32_t ew,eh,ed;uint32_t mipLevels;uint32_t arrayLayers;uint32_t samples;uint32_t tiling;uint32_t usage;uint32_t sharingMode;uint32_t qfiC;const uint32_t*qfiI;uint32_t initialLayout;}ImCI;
            ImCI dici={VK_STRUCTURE_TYPE_IMAGE_CREATE,NULL,0,VK_IMAGE_TYPE_2D,VK_FORMAT_D32_SFLOAT,(uint32_t)ww,(uint32_t)hh,1,1,1,VK_SAMPLE_COUNT_1,VK_IMAGE_TILING_OPTIMAL,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT,0,0,NULL,VK_IMAGE_LAYOUT_UNDEFINED};
            p_vkCreateImage(vk->dev,&dici,NULL,&vk->depthImage);
            typedef struct{VkDeviceSize size;VkDeviceSize alignment;uint32_t memoryTypeBits;}MemReq;
            MemReq dmr;p_vkGetImageMemoryRequirements(vk->dev,vk->depthImage,&dmr);
            typedef struct{uint32_t propertyFlags;uint32_t heapIndex;}MemType;
            typedef struct{VkDeviceSize size;uint32_t flags;}MemHeap;
            typedef struct{uint32_t memoryTypeCount;MemType memoryTypes[32];uint32_t memoryHeapCount;MemHeap memoryHeaps[16];}PhysMemProps;
            PhysMemProps pmp;p_vkGetPhysicalDeviceMemoryProperties(vk->pdev,&pmp);
            uint32_t dmi=0;for(uint32_t i=0;i<pmp.memoryTypeCount;i++){if((dmr.memoryTypeBits&(1<<i))&&(pmp.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL)){dmi=i;break;}}
            typedef struct{uint32_t sType;const void*pNext;VkDeviceSize allocationSize;uint32_t memoryTypeIndex;}MAI;
            MAI dmai={VK_STRUCTURE_TYPE_MEMORY_ALLOCATE,NULL,dmr.size,dmi};
            p_vkAllocateMemory(vk->dev,&dmai,NULL,&vk->depthMem);
            p_vkBindImageMemory(vk->dev,vk->depthImage,vk->depthMem,0);
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;VkImage image;uint32_t viewType;uint32_t format;uint32_t cr,cg,cb,ca;uint32_t aspectMask;uint32_t baseMip;uint32_t mipCount;uint32_t baseLayer;uint32_t layerCount;}DIVCI;
            DIVCI divci={VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE,NULL,0,vk->depthImage,VK_IMAGE_VIEW_TYPE_2D,VK_FORMAT_D32_SFLOAT,0,0,0,0,VK_IMAGE_ASPECT_DEPTH,0,1,0,1};
            p_vkCreateImageView(vk->dev,&divci,NULL,&vk->depthView);}
            /* Render pass (color+depth) */
            typedef struct{uint32_t flags;uint32_t format;uint32_t samples;uint32_t loadOp;uint32_t storeOp;uint32_t stencilLoadOp;uint32_t stencilStoreOp;uint32_t initialLayout;uint32_t finalLayout;}AttDesc;
            AttDesc ad[2]={{0,VK_FORMAT_B8G8R8A8_UNORM,VK_SAMPLE_COUNT_1,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,2,VK_ATTACHMENT_STORE_OP_DONT_CARE,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_PRESENT_SRC},
                {0,VK_FORMAT_D32_SFLOAT,VK_SAMPLE_COUNT_1,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_DONT_CARE,2,VK_ATTACHMENT_STORE_OP_DONT_CARE,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT}};
            typedef struct{uint32_t attachment;uint32_t layout;}AttRef;
            AttRef colRef={0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT},depRef={1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT};
            typedef struct{uint32_t flags;uint32_t pipelineBindPoint;uint32_t inputAttCount;const AttRef*inputAtts;uint32_t colorAttCount;const AttRef*colorAtts;const AttRef*resolveAtts;const AttRef*depthStencilAtt;uint32_t preserveAttCount;const uint32_t*preserveAtts;}SubpassDesc;
            SubpassDesc sp={0,VK_PIPELINE_BIND_POINT_GRAPHICS,0,NULL,1,&colRef,NULL,&depRef,0,NULL};
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;uint32_t attCount;const AttDesc*atts;uint32_t subpassCount;const SubpassDesc*subpasses;uint32_t depCount;const void*deps;}RPCI;
            RPCI rpci={VK_STRUCTURE_TYPE_RENDER_PASS_CREATE,NULL,0,2,ad,1,&sp,0,NULL};
            p_vkCreateRenderPass(vk->dev,&rpci,NULL,&vk->renderPass);
            /* Framebuffers */
            for(uint32_t i=0;i<vk->imageCount;i++){
                VkImageView fbatts[2]={vk->views[i],vk->depthView};
                typedef struct{uint32_t sType;const void*pNext;uint32_t flags;VkRenderPass rp;uint32_t attCount;const VkImageView*atts;uint32_t w,h,layers;}FBCI;
                FBCI fbci={VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE,NULL,0,vk->renderPass,2,fbatts,(uint32_t)ww,(uint32_t)hh,1};
                p_vkCreateFramebuffer(vk->dev,&fbci,NULL,&vk->fbs[i]);}
            /* Shader modules from hex strings */
            int vs_len=0,fs_len=0;
            unsigned char*vs_code=_lp_vk_hex_decode(_lp_vk_vs_hex,&vs_len);
            unsigned char*fs_code=_lp_vk_hex_decode(_lp_vk_fs_hex,&fs_len);
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;size_t codeSize;const uint32_t*pCode;}SMCI;
            VkShaderModule vsm=VK_NULL_HANDLE,fsm=VK_NULL_HANDLE;
            SMCI smci_v={VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE,NULL,0,(size_t)vs_len,(const uint32_t*)vs_code};
            SMCI smci_f={VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE,NULL,0,(size_t)fs_len,(const uint32_t*)fs_code};
            p_vkCreateShaderModule(vk->dev,&smci_v,NULL,&vsm);
            p_vkCreateShaderModule(vk->dev,&smci_f,NULL,&fsm);
            free(vs_code);free(fs_code);
            /* Pipeline */
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;uint32_t stage;VkShaderModule module;const char*pName;const void*pSpec;}PSSCI;
            PSSCI stages[2]={{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE+4,NULL,0,VK_SHADER_STAGE_VERTEX,vsm,"main",NULL},{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE+4,NULL,0,VK_SHADER_STAGE_FRAGMENT,fsm,"main",NULL}};
            /* sType for PSSCI is 18 */
            stages[0].sType=18;stages[1].sType=18;
            typedef struct{uint32_t binding;uint32_t stride;uint32_t inputRate;}VIBind;
            typedef struct{uint32_t location;uint32_t binding;uint32_t format;uint32_t offset;}VIAttr;
            VIBind vib={0,sizeof(LpGuiVertex),0};
            VIAttr via[4]={{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0},{1,0,VK_FORMAT_R32G32B32A32_SFLOAT,16},{2,0,VK_FORMAT_R32G32B32A32_SFLOAT,32},{3,0,VK_FORMAT_R32G32B32A32_SFLOAT,48}};
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;uint32_t bindCount;const VIBind*binds;uint32_t attrCount;const VIAttr*attrs;}VISCI;
            VISCI visci={VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE,NULL,0,1,&vib,4,via};
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;uint32_t topology;VkBool32 primitiveRestart;}IASCI;
            IASCI iasci={VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE,NULL,0,3,0}; /* TRIANGLE_LIST=3 */
            typedef struct{float x,y,w,h,minD,maxD;}Viewport;
            typedef struct{int32_t x,y;uint32_t w,h;}Scissor;
            Viewport vp={0,0,(float)ww,(float)hh,0.0f,1.0f};Scissor sc={0,0,(uint32_t)ww,(uint32_t)hh};
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;uint32_t vpCount;const Viewport*vps;uint32_t scCount;const Scissor*scs;}VSCI;
            VSCI vsci={VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE,NULL,0,1,&vp,1,&sc};
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;VkBool32 depthClamp;VkBool32 rastDiscard;uint32_t polyMode;uint32_t cullMode;uint32_t frontFace;VkBool32 depthBias;float dbConst,dbClamp,dbSlope;float lineWidth;}RSCI;
            RSCI rsci;ZeroMemory(&rsci,sizeof(rsci));rsci.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE;rsci.polyMode=0;rsci.cullMode=2;rsci.frontFace=1;rsci.lineWidth=1.0f;
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;uint32_t rasterSamples;VkBool32 sampleShading;float minSampleShading;const void*pSampleMask;VkBool32 alphaToCoverage;VkBool32 alphaToOne;}MSCI;
            MSCI msci={VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE,NULL,0,VK_SAMPLE_COUNT_1,0,1.0f,NULL,0,0};
            typedef struct{VkBool32 blendEnable;uint32_t srcColor,dstColor,colorOp,srcAlpha,dstAlpha,alphaOp;uint32_t writeMask;}CBAS;
            CBAS cbas={0,0,0,0,0,0,0,VK_COLOR_COMPONENT_ALL};
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;VkBool32 logicOpEnable;uint32_t logicOp;uint32_t attCount;const CBAS*atts;float blendConstants[4];}CBSCI;
            CBSCI cbsci={VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE,NULL,0,0,0,1,&cbas,{0,0,0,0}};
            uint32_t dynStates[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;uint32_t dynStateCount;const uint32_t*dynStates;}DSCI;
            DSCI dsci={VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE,NULL,0,2,dynStates};
            /* Depth stencil state */
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;VkBool32 depthTest;VkBool32 depthWrite;uint32_t depthCompare;VkBool32 boundsTest;VkBool32 stencilTest;uint32_t front[7];uint32_t back[7];float minBounds;float maxBounds;}DSSCI;
            DSSCI dssci;ZeroMemory(&dssci,sizeof(dssci));dssci.sType=25;dssci.depthTest=1;dssci.depthWrite=1;dssci.depthCompare=VK_COMPARE_OP_LESS;
            /* Push constants for MVP+lighting (same layout as LpDxCB) */
            typedef struct{uint32_t stageFlags;uint32_t offset;uint32_t size;}PCRange;
            PCRange pcr={VK_SHADER_STAGE_VERTEX|VK_SHADER_STAGE_FRAGMENT,0,sizeof(LpDxCB)};
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;uint32_t setLayoutCount;const void*pSetLayouts;uint32_t pushConstantRangeCount;const PCRange*pushConstantRanges;}PLCI;
            PLCI plci={VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE,NULL,0,0,NULL,1,&pcr};
            p_vkCreatePipelineLayout(vk->dev,&plci,NULL,&vk->playout);
            /* Graphics pipeline create info */
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;uint32_t stageCount;const PSSCI*pStages;const VISCI*pVertexInputState;const IASCI*pInputAssemblyState;const void*pTessellationState;const VSCI*pViewportState;const RSCI*pRasterizationState;const MSCI*pMultisampleState;const DSSCI*pDepthStencilState;const CBSCI*pColorBlendState;const DSCI*pDynamicState;VkPipelineLayout layout;VkRenderPass renderPass;uint32_t subpass;VkPipeline basePipelineHandle;int32_t basePipelineIndex;}GPCI;
            GPCI gpci={VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE,NULL,0,2,stages,&visci,&iasci,NULL,&vsci,&rsci,&msci,&dssci,&cbsci,&dsci,vk->playout,vk->renderPass,0,VK_NULL_HANDLE,-1};
            p_vkCreateGraphicsPipelines(vk->dev,VK_NULL_HANDLE,1,&gpci,NULL,&vk->pipeline);
            p_vkDestroyShaderModule(vk->dev,vsm,NULL);p_vkDestroyShaderModule(vk->dev,fsm,NULL);
            /* Command pool + buffer */
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;uint32_t queueFamilyIndex;}CPCI;
            CPCI cpci={VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE,NULL,VK_COMMAND_POOL_CREATE_RESET,vk->qfi};
            p_vkCreateCommandPool(vk->dev,&cpci,NULL,&vk->cmdPool);
            typedef struct{uint32_t sType;const void*pNext;VkCommandPool commandPool;uint32_t level;uint32_t commandBufferCount;}CBAI;
            CBAI cbai={VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE,NULL,vk->cmdPool,VK_COMMAND_BUFFER_LEVEL_PRIMARY,1};
            p_vkAllocateCommandBuffers(vk->dev,&cbai,&vk->cmdBuf);
            /* Sync */
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;}SemCI;
            SemCI semci={VK_STRUCTURE_TYPE_SEMAPHORE_CREATE,NULL,0};
            p_vkCreateSemaphore(vk->dev,&semci,NULL,&vk->semAvail);
            p_vkCreateSemaphore(vk->dev,&semci,NULL,&vk->semDone);
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;}FenCI;
            FenCI fci={VK_STRUCTURE_TYPE_FENCE_CREATE,NULL,VK_FENCE_CREATE_SIGNALED};
            p_vkCreateFence(vk->dev,&fci,NULL,&vk->fence);
            /* Vertex buffer (dynamic, host-visible) */
            vk->vbufSize=sizeof(LpGuiVertex)*65536;
            typedef struct{uint32_t sType;const void*pNext;uint32_t flags;VkDeviceSize size;uint32_t usage;uint32_t sharingMode;uint32_t qfiC;const uint32_t*qfiI;}BufCI;
            BufCI bci={VK_STRUCTURE_TYPE_BUFFER_CREATE,NULL,0,(VkDeviceSize)vk->vbufSize,VK_BUFFER_USAGE_VERTEX_BUFFER,0,0,NULL};
            p_vkCreateBuffer(vk->dev,&bci,NULL,&vk->vbuf);
            typedef struct{VkDeviceSize size;VkDeviceSize alignment;uint32_t memoryTypeBits;}MemReqB;
            MemReqB bmr;p_vkGetBufferMemoryRequirements(vk->dev,vk->vbuf,&bmr);
            typedef struct{uint32_t propertyFlags;uint32_t heapIndex;}MemTypeB;
            typedef struct{VkDeviceSize size;uint32_t flags;}MemHeapB;
            typedef struct{uint32_t memoryTypeCount;MemTypeB memoryTypes[32];uint32_t memoryHeapCount;MemHeapB memoryHeaps[16];}PhysMemPropsB;
            PhysMemPropsB pmpb;p_vkGetPhysicalDeviceMemoryProperties(vk->pdev,&pmpb);
            uint32_t bmi=0;for(uint32_t i=0;i<pmpb.memoryTypeCount;i++){if((bmr.memoryTypeBits&(1<<i))&&(pmpb.memoryTypes[i].propertyFlags&(VK_MEMORY_PROPERTY_HOST_VISIBLE|VK_MEMORY_PROPERTY_HOST_COHERENT))==(VK_MEMORY_PROPERTY_HOST_VISIBLE|VK_MEMORY_PROPERTY_HOST_COHERENT)){bmi=i;break;}}
            typedef struct{uint32_t sType;const void*pNext;VkDeviceSize allocationSize;uint32_t memoryTypeIndex;}MAIB;
            MAIB bmai={VK_STRUCTURE_TYPE_MEMORY_ALLOCATE,NULL,bmr.size,bmi};
            p_vkAllocateMemory(vk->dev,&bmai,NULL,&vk->vbufMem);
            p_vkBindBufferMemory(vk->dev,vk->vbuf,vk->vbufMem,0);
            p_vkMapMemory(vk->dev,vk->vbufMem,0,(VkDeviceSize)vk->vbufSize,0,&vk->vbufMap);
            /* Register canvas */
            LpCanvas3D*g3=&_lp_c3d[_lp_c3d_n++];ZeroMemory(g3,sizeof(*g3));g3->hwnd=hw;g3->w=ww;g3->h=hh;g3->backend=LP_GUI_BACKEND_VULKAN;
            _lp_active3d=g3;_lp_vk_active=vk;_lp_vk_n++;_lp_gui_backend_active=LP_GUI_BACKEND_VULKAN;_lp_m4_init();
            fprintf(stderr,"[LP GUI] Vulkan GPU backend initialized successfully.\n");
            return(int)(intptr_t)hw;
        }
        fprintf(stderr,"[LP GUI] Vulkan init failed, falling back to OpenGL.\n");
        if(vklib)FreeLibrary(vklib);
        vk_fail_dev: vk_fail_inst: vk_fail:;
    }
    if(!_lp_hInst)_lp_hInst=GetModuleHandle(NULL);
    if(!_lp_gl_cls_reg){WNDCLASSA wc={0};wc.lpfnWndProc=_lp_gui_wndproc;wc.hInstance=_lp_hInst;
        wc.lpszClassName="LpGL";wc.style=CS_HREDRAW|CS_VREDRAW|CS_OWNDC;wc.hCursor=LoadCursor(NULL,IDC_ARROW);
        RegisterClassA(&wc);_lp_gl_cls_reg=1;}
    int ww=w>0?w:640,hh=h>0?h:480;
    HWND hw=CreateWindowExA(0,"LpGL","",WS_CHILD|WS_VISIBLE,x,y,ww,hh,(HWND)(intptr_t)p,NULL,_lp_hInst,NULL);
    if(_lp_c3d_n<LP_GUI_MAX){
        LpCanvas3D*g=&_lp_c3d[_lp_c3d_n++];ZeroMemory(g,sizeof(*g));g->hwnd=hw;g->w=ww;g->h=hh;g->backend=LP_GUI_BACKEND_OPENGL;
        g->hdc=GetDC(hw);
        PIXELFORMATDESCRIPTOR pfd={sizeof(pfd),1,PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
            PFD_TYPE_RGBA,32,0,0,0,0,0,0,0,0,0,0,0,0,0,24,8,0,PFD_MAIN_PLANE,0,0,0,0};
        int fmt=ChoosePixelFormat(g->hdc,&pfd);SetPixelFormat(g->hdc,fmt,&pfd);
        g->hglrc=wglCreateContext(g->hdc);wglMakeCurrent(g->hdc,g->hglrc);
        /* Load VSync extension */
        if(!_lp_wglSwapInterval)_lp_wglSwapInterval=(PFNWGLSWAPINTERVALEXT)wglGetProcAddress("wglSwapIntervalEXT");
        glEnable(GL_DEPTH_TEST);glClearColor(0,0,0,1);glViewport(0,0,ww,hh);
        /* Default perspective */
        _lp_active3d=g;_lp_m4_init();
        glMatrixMode(GL_PROJECTION);glLoadIdentity();
        gluPerspective(45.0,(double)ww/(double)hh,0.1,1000.0);
        glMatrixMode(GL_MODELVIEW);glLoadIdentity();_lp_gui_backend_active=LP_GUI_BACKEND_OPENGL;
    }
    return(int)(intptr_t)hw;}
static inline int lp_gui_canvasdx11(int p,int x,int y,int w,int h){return _lp_gui_canvasdx11(p,x,y,w,h);}

/* 3D Rendering */
static inline void lp_gui_clear3d(int cv,double r,double g,double b){
    LpCanvas3D*c=_lp_fc3d((HWND)(intptr_t)cv);if(!c)return;
    _lp_active3d=c;_lp_gui_backend_active=c->backend;
    if(c->backend==LP_GUI_BACKEND_DIRECTX11&&c->dx_ctx&&c->dx_rtv){
        FLOAT clr[4]={(FLOAT)r,(FLOAT)g,(FLOAT)b,1.0f};ID3D11DeviceContext_ClearRenderTargetView(c->dx_ctx,c->dx_rtv,clr);
        if(c->dx_dsv)ID3D11DeviceContext_ClearDepthStencilView(c->dx_ctx,c->dx_dsv,D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL,1.0f,0);return;}
    if(c->backend==LP_GUI_BACKEND_VULKAN&&_lp_vk_active){_lp_gui_batch_n=0;
        _LpVkCtx*vk=_lp_vk_active;
        _lp_vk_clear_r=(float)r;_lp_vk_clear_g=(float)g;_lp_vk_clear_b=(float)b;
        return;}
    wglMakeCurrent(c->hdc,c->hglrc);glClearColor((float)r,(float)g,(float)b,1.0f);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);}
static inline void lp_gui_present3d(int cv){
    LpCanvas3D*c=_lp_fc3d((HWND)(intptr_t)cv);if(!c)return;
    if(c->backend==LP_GUI_BACKEND_DIRECTX11&&c->dx_swap){IDXGISwapChain_Present(c->dx_swap,_lp_gui_vsync?1:0,0);return;}
    if(c->backend==LP_GUI_BACKEND_VULKAN&&_lp_vk_active){
        _LpVkCtx*vk=_lp_vk_active;
        p_vkWaitForFences(vk->dev,1,&vk->fence,1,(uint64_t)-1);p_vkResetFences(vk->dev,1,&vk->fence);
        uint32_t imgIdx=0;p_vkAcquireNextImageKHR(vk->dev,vk->swapchain,(uint64_t)-1,vk->semAvail,VK_NULL_HANDLE,&imgIdx);
        /* Upload vertices */
        if(_lp_gui_batch_n>0&&vk->vbufMap){
            int sz=_lp_gui_batch_n*(int)sizeof(LpGuiVertex);if((uint32_t)sz>vk->vbufSize)sz=(int)vk->vbufSize;
            memcpy(vk->vbufMap,_lp_gui_batch,(size_t)sz);}
        p_vkResetCommandBuffer(vk->cmdBuf,0);
        typedef struct{uint32_t sType;const void*pNext;uint32_t flags;}CBBI;
        CBBI cbbi={VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN,NULL,VK_COMMAND_BUFFER_USAGE_ONE_TIME};
        p_vkBeginCommandBuffer(vk->cmdBuf,&cbbi);
        /* Begin render pass */
        typedef struct{float r,g,b,a;}ClearColor;typedef struct{float depth;uint32_t stencil;}ClearDS;
        typedef union{ClearColor color;ClearDS depthStencil;}ClearValue;
        ClearValue cv[2];cv[0].color.r=_lp_vk_clear_r;cv[0].color.g=_lp_vk_clear_g;cv[0].color.b=_lp_vk_clear_b;cv[0].color.a=1.0f;cv[1].depthStencil.depth=1.0f;cv[1].depthStencil.stencil=0;
        typedef struct{uint32_t sType;const void*pNext;VkRenderPass renderPass;VkFramebuffer framebuffer;int32_t ox,oy;uint32_t ew,eh;uint32_t clearValueCount;const ClearValue*pClearValues;}RPBI;
        RPBI rpbi={VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN,NULL,vk->renderPass,vk->fbs[imgIdx],0,0,(uint32_t)c->w,(uint32_t)c->h,2,cv};
        p_vkCmdBeginRenderPass(vk->cmdBuf,&rpbi,VK_SUBPASS_CONTENTS_INLINE);
        p_vkCmdBindPipeline(vk->cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS,vk->pipeline);
        /* Dynamic viewport/scissor */
        typedef struct{float x,y,w,h,minD,maxD;}VP;typedef struct{int32_t x,y;uint32_t w,h;}SC;
        VP vp2={0,0,(float)c->w,(float)c->h,0,1};SC sc2={0,0,(uint32_t)c->w,(uint32_t)c->h};
        p_vkCmdSetViewport(vk->cmdBuf,0,1,&vp2);p_vkCmdSetScissor(vk->cmdBuf,0,1,&sc2);
        /* Push constants: MVP + lighting */
        _lp_m4_init();
        LpGuiMat4 mv=_lp_m4_mul(_lp_gui_view,_lp_gui_model);LpGuiMat4 mvp=_lp_m4_mul(_lp_gui_proj,mv);
        LpDxCB pc;for(int r2=0;r2<4;r2++)for(int c2=0;c2<4;c2++)pc.mvp[c2*4+r2]=mvp.m[c2*4+r2];
        pc.lightPos[0]=_lp_gui_lx;pc.lightPos[1]=_lp_gui_ly;pc.lightPos[2]=_lp_gui_lz;pc.lightPos[3]=1;
        pc.lightCol[0]=_lp_gui_lr;pc.lightCol[1]=_lp_gui_lg;pc.lightCol[2]=_lp_gui_lb;pc.lightCol[3]=1;
        pc.ambient[0]=pc.ambient[1]=pc.ambient[2]=_lp_gui_ambient;pc.ambient[3]=1;
        pc.flags[0]=_lp_gui_lighting;pc.flags[1]=_lp_gui_texturing;pc.flags[2]=pc.flags[3]=0;
        p_vkCmdPushConstants(vk->cmdBuf,vk->playout,VK_SHADER_STAGE_VERTEX|VK_SHADER_STAGE_FRAGMENT,0,sizeof(pc),&pc);
        /* Bind vertex buffer and draw */
        VkDeviceSize vbOff=0;p_vkCmdBindVertexBuffers(vk->cmdBuf,0,1,&vk->vbuf,&vbOff);
        if(_lp_gui_batch_n>0)p_vkCmdDraw(vk->cmdBuf,(uint32_t)_lp_gui_batch_n,1,0,0);
        p_vkCmdEndRenderPass(vk->cmdBuf);p_vkEndCommandBuffer(vk->cmdBuf);
        /* Submit */
        uint32_t waitStage=0x400; /* VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT */
        typedef struct{uint32_t sType;const void*pNext;uint32_t waitSemCount;const VkSemaphore*pWaitSem;const uint32_t*pWaitStages;uint32_t cmdBufCount;const VkCommandBuffer*pCmdBufs;uint32_t sigSemCount;const VkSemaphore*pSigSem;}SubInfo;
        SubInfo si={VK_STRUCTURE_TYPE_SUBMIT_INFO,NULL,1,&vk->semAvail,&waitStage,1,&vk->cmdBuf,1,&vk->semDone};
        p_vkQueueSubmit(vk->queue,1,&si,vk->fence);
        /* Present */
        typedef struct{uint32_t sType;const void*pNext;uint32_t waitSemCount;const VkSemaphore*pWaitSems;uint32_t swapchainCount;const VkSwapchainKHR*pSwapchains;const uint32_t*pImageIndices;int32_t*pResults;}PresentInfo;
        PresentInfo pi={VK_STRUCTURE_TYPE_PRESENT_INFO,NULL,1,&vk->semDone,1,&vk->swapchain,&imgIdx,NULL};
        p_vkQueuePresentKHR(vk->queue,&pi);
        return;}
    SwapBuffers(c->hdc);}
static inline void lp_gui_begin3d(int mode){
    _lp_gui_mode=mode;_lp_gui_batch_n=0;_lp_gui_quad_n=0;
    if(mode==2)_lp_gui_topology=D3D11_PRIMITIVE_TOPOLOGY_LINELIST;else if(mode==3)_lp_gui_topology=D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;else _lp_gui_topology=D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;
    GLenum m=GL_TRIANGLES;
    if(mode==1)m=GL_QUADS;else if(mode==2)m=GL_LINES;else if(mode==3)m=GL_POINTS;else if(mode==4)m=GL_LINE_LOOP;else if(mode==5)m=GL_TRIANGLE_FAN;else if(mode==6)m=GL_TRIANGLE_STRIP;
    glBegin(m);}
static inline void lp_gui_end3d(void){if(_lp_gui_backend_active==LP_GUI_BACKEND_DIRECTX11){_lp_gui_dx_draw_batch();return;}if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glEnd();}
static inline void lp_gui_vertex3d(double x,double y,double z){if(_lp_gui_backend_active==LP_GUI_BACKEND_DIRECTX11||_lp_gui_backend_active==LP_GUI_BACKEND_VULKAN){_lp_gui_emit_vertex((float)x,(float)y,(float)z);return;}if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glVertex3f((float)x,(float)y,(float)z);}
static inline void lp_gui_color3d(double r,double g,double b){_lp_gui_cr=(float)r;_lp_gui_cg=(float)g;_lp_gui_cb=(float)b;_lp_gui_ca=1.0f;if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glColor3f((float)r,(float)g,(float)b);}
static inline void lp_gui_color4d(double r,double g,double b,double a){_lp_gui_cr=(float)r;_lp_gui_cg=(float)g;_lp_gui_cb=(float)b;_lp_gui_ca=(float)a;if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glColor4f((float)r,(float)g,(float)b,(float)a);}
static inline void lp_gui_normal3d(double x,double y,double z){_lp_gui_nx=(float)x;_lp_gui_ny=(float)y;_lp_gui_nz=(float)z;if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glNormal3f((float)x,(float)y,(float)z);}
static inline void lp_gui_texcoord2d(double u,double v){if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glTexCoord2f((float)u,(float)v);}
/* Camera & Transform */
static inline void lp_gui_perspective(double fov,double aspect,double near_,double far_){
    _lp_m4_init();_lp_gui_proj=_lp_m4_perspective((float)fov,(float)aspect,(float)near_,(float)far_);
    if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glMatrixMode(GL_PROJECTION);glLoadIdentity();gluPerspective(fov,aspect,near_,far_);glMatrixMode(GL_MODELVIEW);}
static inline void lp_gui_ortho(double l,double r,double b,double t,double n,double f){
    _lp_m4_init();_lp_gui_proj=_lp_m4_ortho((float)l,(float)r,(float)b,(float)t,(float)n,(float)f);
    if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(l,r,b,t,n,f);glMatrixMode(GL_MODELVIEW);}
static inline void lp_gui_look_at(double ex,double ey,double ez,double cx,double cy,double cz,double ux,double uy,double uz){
    _lp_m4_init();_lp_gui_view=_lp_m4_lookat((float)ex,(float)ey,(float)ez,(float)cx,(float)cy,(float)cz,(float)ux,(float)uy,(float)uz);
    if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glLoadIdentity();gluLookAt(ex,ey,ez,cx,cy,cz,ux,uy,uz);}
static inline void lp_gui_translate3d(double x,double y,double z){_lp_m4_init();_lp_gui_model=_lp_m4_mul(_lp_gui_model,_lp_m4_translate((float)x,(float)y,(float)z));if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glTranslatef((float)x,(float)y,(float)z);}
static inline void lp_gui_rotate3d(double a,double x,double y,double z){_lp_m4_init();_lp_gui_model=_lp_m4_mul(_lp_gui_model,_lp_m4_rotate((float)a,(float)x,(float)y,(float)z));if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glRotatef((float)a,(float)x,(float)y,(float)z);}
static inline void lp_gui_scale3d(double x,double y,double z){_lp_m4_init();_lp_gui_model=_lp_m4_mul(_lp_gui_model,_lp_m4_scale((float)x,(float)y,(float)z));if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glScalef((float)x,(float)y,(float)z);}
static inline void lp_gui_push_matrix(void){_lp_m4_init();if(_lp_gui_stack_n<32)_lp_gui_stack[_lp_gui_stack_n++]=_lp_gui_model;if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glPushMatrix();}
static inline void lp_gui_pop_matrix(void){_lp_m4_init();if(_lp_gui_stack_n>0)_lp_gui_model=_lp_gui_stack[--_lp_gui_stack_n];if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glPopMatrix();}
static inline void lp_gui_identity(void){_lp_m4_init();_lp_m4_identity(&_lp_gui_model);if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glLoadIdentity();}
static inline void lp_gui_mode_projection(void){if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glMatrixMode(GL_PROJECTION);}
static inline void lp_gui_mode_modelview(void){if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glMatrixMode(GL_MODELVIEW);}

/* 3D Primitives */
static inline void lp_gui_draw_cube(double x,double y,double z,double s,int color){
    float r=((color)&0xFF)/255.0f,g=((color>>8)&0xFF)/255.0f,b=((color>>16)&0xFF)/255.0f;
    float hs=(float)s*0.5f;
    if(_lp_gui_backend_active==LP_GUI_BACKEND_DIRECTX11){
        float vx[8][3]={{-hs,-hs,hs},{hs,-hs,hs},{hs,hs,hs},{-hs,hs,hs},{-hs,-hs,-hs},{hs,-hs,-hs},{hs,hs,-hs},{-hs,hs,-hs}};
        int idx[36]={0,1,2,0,2,3,5,4,7,5,7,6,3,2,6,3,6,7,4,5,1,4,1,0,1,5,6,1,6,2,4,0,3,4,3,7};
        lp_gui_push_matrix();lp_gui_translate3d(x,y,z);lp_gui_color3d(r,g,b);lp_gui_begin3d(0);
        for(int i=0;i<36;i++)lp_gui_vertex3d(vx[idx[i]][0],vx[idx[i]][1],vx[idx[i]][2]);
        lp_gui_end3d();lp_gui_pop_matrix();return;
    }
    if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;
    glPushMatrix();glTranslatef((float)x,(float)y,(float)z);
    glBegin(GL_QUADS);
    glColor3f(r,g,b);
    /* Front */  glNormal3f(0,0,1);glVertex3f(-hs,-hs,hs);glVertex3f(hs,-hs,hs);glVertex3f(hs,hs,hs);glVertex3f(-hs,hs,hs);
    /* Back */   glNormal3f(0,0,-1);glVertex3f(hs,-hs,-hs);glVertex3f(-hs,-hs,-hs);glVertex3f(-hs,hs,-hs);glVertex3f(hs,hs,-hs);
    /* Top */    glNormal3f(0,1,0);glVertex3f(-hs,hs,hs);glVertex3f(hs,hs,hs);glVertex3f(hs,hs,-hs);glVertex3f(-hs,hs,-hs);
    /* Bottom */ glNormal3f(0,-1,0);glVertex3f(-hs,-hs,-hs);glVertex3f(hs,-hs,-hs);glVertex3f(hs,-hs,hs);glVertex3f(-hs,-hs,hs);
    /* Right */  glNormal3f(1,0,0);glVertex3f(hs,-hs,hs);glVertex3f(hs,-hs,-hs);glVertex3f(hs,hs,-hs);glVertex3f(hs,hs,hs);
    /* Left */   glNormal3f(-1,0,0);glVertex3f(-hs,-hs,-hs);glVertex3f(-hs,-hs,hs);glVertex3f(-hs,hs,hs);glVertex3f(-hs,hs,-hs);
    glEnd();glPopMatrix();}
static inline void lp_gui_draw_sphere(double x,double y,double z,double radius,int slices,int stacks,int color){
    float cr=((color)&0xFF)/255.0f,cg=((color>>8)&0xFF)/255.0f,cb=((color>>16)&0xFF)/255.0f;
    int sl=slices>0?slices:16,st=stacks>0?stacks:16;
    if(_lp_gui_backend_active==LP_GUI_BACKEND_DIRECTX11){
        /* CPU sphere triangulation for DX11 */
        lp_gui_push_matrix();lp_gui_translate3d(x,y,z);lp_gui_color3d(cr,cg,cb);lp_gui_begin3d(0);
        for(int i=0;i<st;i++)for(int j=0;j<sl;j++){
            float t0=(float)i/st,t1=(float)(i+1)/st,p0=(float)j/sl,p1=(float)(j+1)/sl;
            float phi0=3.14159265f*t0,phi1=3.14159265f*t1,th0=6.28318530f*p0,th1=6.28318530f*p1;
            float r0=sinf(phi0)*(float)radius,y0=cosf(phi0)*(float)radius,r1=sinf(phi1)*(float)radius,y1=cosf(phi1)*(float)radius;
            float ax=r0*sinf(th0),az=r0*cosf(th0),bx=r1*sinf(th0),bz=r1*cosf(th0);
            float cx_=r1*sinf(th1),cz=r1*cosf(th1),dx=r0*sinf(th1),dz=r0*cosf(th1);
            lp_gui_vertex3d(ax,y0,az);lp_gui_vertex3d(bx,y1,bz);lp_gui_vertex3d(cx_,y1,cz);
            lp_gui_vertex3d(ax,y0,az);lp_gui_vertex3d(cx_,y1,cz);lp_gui_vertex3d(dx,y0,dz);
        }
        lp_gui_end3d();lp_gui_pop_matrix();return;
    }
    if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;
    glPushMatrix();glTranslatef((float)x,(float)y,(float)z);glColor3f(cr,cg,cb);
    GLUquadric*q=gluNewQuadric();gluQuadricNormals(q,GLU_SMOOTH);
    gluSphere(q,radius,sl,st);gluDeleteQuadric(q);glPopMatrix();}
static inline void lp_gui_draw_plane(double y,double sz,int color){
    float r=((color)&0xFF)/255.0f,g=((color>>8)&0xFF)/255.0f,b=((color>>16)&0xFF)/255.0f;
    float hs=(float)sz*0.5f;
    if(_lp_gui_backend_active==LP_GUI_BACKEND_DIRECTX11){
        lp_gui_color3d(r,g,b);lp_gui_begin3d(0);
        lp_gui_vertex3d(-hs,y,-hs);lp_gui_vertex3d(hs,y,-hs);lp_gui_vertex3d(hs,y,hs);
        lp_gui_vertex3d(-hs,y,-hs);lp_gui_vertex3d(hs,y,hs);lp_gui_vertex3d(-hs,y,hs);
        lp_gui_end3d();return;
    }
    if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;
    glBegin(GL_QUADS);glColor3f(r,g,b);glNormal3f(0,1,0);
    glVertex3f(-hs,(float)y,-hs);glVertex3f(hs,(float)y,-hs);glVertex3f(hs,(float)y,hs);glVertex3f(-hs,(float)y,hs);
    glEnd();}
static inline void lp_gui_draw_grid(double y,double sz,int div,int color){
    float r=((color)&0xFF)/255.0f,g=((color>>8)&0xFF)/255.0f,b=((color>>16)&0xFF)/255.0f;
    float hs=(float)sz*0.5f;float step=(float)sz/(div>0?div:10);
    if(_lp_gui_backend_active==LP_GUI_BACKEND_DIRECTX11){
        lp_gui_color3d(r,g,b);lp_gui_begin3d(2);
        for(float i=-hs;i<=hs+0.001f;i+=step){lp_gui_vertex3d(i,y,-hs);lp_gui_vertex3d(i,y,hs);lp_gui_vertex3d(-hs,y,i);lp_gui_vertex3d(hs,y,i);}
        lp_gui_end3d();return;
    }
    if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;
    glBegin(GL_LINES);glColor3f(r,g,b);
    for(float i=-hs;i<=hs+0.001f;i+=step){glVertex3f(i,(float)y,-hs);glVertex3f(i,(float)y,hs);glVertex3f(-hs,(float)y,i);glVertex3f(hs,(float)y,i);}
    glEnd();}
/* Lighting */
static inline void lp_gui_enable_lighting(void){_lp_gui_lighting=1;if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glEnable(GL_LIGHTING);glEnable(GL_COLOR_MATERIAL);glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);}
static inline void lp_gui_disable_lighting(void){_lp_gui_lighting=0;if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glDisable(GL_LIGHTING);glDisable(GL_COLOR_MATERIAL);}
static inline void lp_gui_set_light(int id,double x,double y,double z,double r,double g,double b){
    if(id==0){_lp_gui_lx=(float)x;_lp_gui_ly=(float)y;_lp_gui_lz=(float)z;_lp_gui_lr=(float)r;_lp_gui_lg=(float)g;_lp_gui_lb=(float)b;}
    if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;
    GLenum light=GL_LIGHT0+id;glEnable(light);
    float pos[4]={(float)x,(float)y,(float)z,1.0f};float col[4]={(float)r,(float)g,(float)b,1.0f};
    float amb[4]={(float)r*0.2f,(float)g*0.2f,(float)b*0.2f,1.0f};
    glLightfv(light,GL_POSITION,pos);glLightfv(light,GL_DIFFUSE,col);glLightfv(light,GL_AMBIENT,amb);}
static inline void lp_gui_enable_depth_test(void){
    if(_lp_gui_backend_active==LP_GUI_BACKEND_DIRECTX11&&_lp_active3d&&_lp_active3d->dx_dss){ID3D11DeviceContext_OMSetDepthStencilState(_lp_active3d->dx_ctx,_lp_active3d->dx_dss,0);return;}
    if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glEnable(GL_DEPTH_TEST);}
static inline void lp_gui_disable_depth_test(void){if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glDisable(GL_DEPTH_TEST);}
/* Texture */
static inline int lp_gui_load_texture(const char*path){
    if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return 0;
    if(!path)return 0;
    HBITMAP bm=(HBITMAP)LoadImageA(NULL,path,IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
    if(!bm)return 0;
    BITMAP bi;GetObject(bm,sizeof(bi),&bi);
    GLuint tex;glGenTextures(1,&tex);glBindTexture(GL_TEXTURE_2D,tex);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    /* Convert BMP to RGB */
    int w=bi.bmWidth,h=bi.bmHeight;
    unsigned char*pixels=(unsigned char*)malloc(w*h*3);
    if(pixels){
        unsigned char*src=(unsigned char*)bi.bmBits;
        int stride=((w*bi.bmBitsPixel/8+3)&~3);
        for(int row=0;row<h;row++)for(int col=0;col<w;col++){
            int si=(h-1-row)*stride+col*(bi.bmBitsPixel/8);
            int di=(row*w+col)*3;
            pixels[di]=src[si+2];pixels[di+1]=src[si+1];pixels[di+2]=src[si];}
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,pixels);free(pixels);}
    DeleteObject(bm);return(int)tex;}
static inline void lp_gui_bind_texture(int id){if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,(GLuint)id);}
static inline void lp_gui_unbind_texture(void){if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;glDisable(GL_TEXTURE_2D);}
/* GPU Info */
static inline void lp_gui_prefer_discrete_gpu(void){/* Already handled by NvOptimusEnablement + AmdPowerXpress exports */}
static inline const char*lp_gui_gpu_info(void){
    if(_lp_gui_backend_active==LP_GUI_BACKEND_DIRECTX11){lp_gui_directx_available();return _lp_gui_dx_name[0]?_lp_gui_dx_name:"DirectX adapter";}
    if(_lp_gui_backend_active==LP_GUI_BACKEND_VULKAN){lp_gui_vulkan_available();return _lp_gui_vk_info;}
    return(const char*)glGetString(GL_RENDERER);}
static inline const char*lp_gui_gpu_vendor(void){
    if(_lp_gui_backend_active==LP_GUI_BACKEND_DIRECTX11){lp_gui_directx_available();return _lp_gui_dx_vendor;}
    if(_lp_gui_backend_active==LP_GUI_BACKEND_VULKAN)return"Vulkan";
    return(const char*)glGetString(GL_VENDOR);}
static inline const char*lp_gui_gpu_version(void){
    if(_lp_gui_backend_active==LP_GUI_BACKEND_DIRECTX11)return"D3D11";
    if(_lp_gui_backend_active==LP_GUI_BACKEND_VULKAN)return lp_gui_vulkan_available()?"Vulkan loader":"Vulkan unavailable";
    return(const char*)glGetString(GL_VERSION);}
static inline void lp_gui_set_vsync(int on){_lp_gui_vsync=on?1:0;if(_lp_wglSwapInterval)_lp_wglSwapInterval(on);}
static inline void lp_gui_set_msaa(int samples){
    if(_lp_gui_backend_active!=LP_GUI_BACKEND_OPENGL)return;
    if(samples>1){glEnable(GL_MULTISAMPLE);}else{glDisable(GL_MULTISAMPLE);}}

#else
/* ══════════ Linux/macOS Fallback (console stubs) ══════════ */
static inline int lp_gui_rgb(int r,int g,int b){return(r|(g<<8)|(b<<16));}
static inline int lp_gui_window(const char*t,int w,int h){printf("[GUI] Window: %s (%dx%d)\n",t?t:"LP GUI",w,h);return 1;}
static inline void lp_gui_close(int h){(void)h;}
static inline void lp_gui_set_title(int h,const char*t){(void)h;(void)t;}
static inline int lp_gui_label(int p,const char*t,int x,int y){printf("[GUI] Label: %s\n",t?t:"");(void)p;(void)x;(void)y;return 1;}
static inline int lp_gui_button(int p,const char*t,int x,int y,int w,int h){printf("[GUI] Button: %s\n",t?t:"");(void)p;(void)x;(void)y;(void)w;(void)h;return 1;}
static inline int lp_gui_input(int p,int x,int y,int w,int h){(void)p;(void)x;(void)y;(void)w;(void)h;return 1;}
static inline int lp_gui_textarea(int p,int x,int y,int w,int h){(void)p;(void)x;(void)y;(void)w;(void)h;return 1;}
static inline int lp_gui_checkbox(int p,const char*t,int x,int y){(void)p;(void)t;(void)x;(void)y;return 1;}
static inline int lp_gui_radio(int p,const char*t,int x,int y){(void)p;(void)t;(void)x;(void)y;return 1;}
static inline int lp_gui_group(int p,const char*t,int x,int y,int w,int h){(void)p;(void)t;(void)x;(void)y;(void)w;(void)h;return 1;}
static inline int lp_gui_dropdown(int p,int x,int y,int w,int h){(void)p;(void)x;(void)y;(void)w;(void)h;return 1;}
static inline int lp_gui_listbox(int p,int x,int y,int w,int h){(void)p;(void)x;(void)y;(void)w;(void)h;return 1;}
static inline int lp_gui_slider(int p,int x,int y,int w,int h,int mn,int mx){(void)p;(void)x;(void)y;(void)w;(void)h;(void)mn;(void)mx;return 1;}
static inline int lp_gui_progress(int p,int x,int y,int w,int h){(void)p;(void)x;(void)y;(void)w;(void)h;return 1;}
static inline int lp_gui_tab(int p,int x,int y,int w,int h){(void)p;(void)x;(void)y;(void)w;(void)h;return 1;}
static inline int lp_gui_image(int p,const char*path,int x,int y,int w,int h){(void)p;(void)path;(void)x;(void)y;(void)w;(void)h;return 1;}
static inline void lp_gui_set_text(int h,const char*t){(void)h;(void)t;}
static inline const char*lp_gui_get_text(int h){(void)h;return strdup("");}
static inline void lp_gui_set_value(int h,int v){(void)h;(void)v;}
static inline int lp_gui_get_value(int h){(void)h;return 0;}
static inline void lp_gui_add_item(int h,const char*t){(void)h;(void)t;}
static inline void lp_gui_set_visible(int h,int v){(void)h;(void)v;}
static inline void lp_gui_set_enabled(int h,int e){(void)h;(void)e;}
static inline void lp_gui_set_font(int h,const char*n,int s){(void)h;(void)n;(void)s;}
static inline void lp_gui_set_bg_color(int h,int c){(void)h;(void)c;}
static inline void lp_gui_on_timer(void(*cb)(void)){(void)cb;}
static inline void lp_gui_on_key_down(void(*cb)(int)){(void)cb;}
static inline void lp_gui_on_key_up(void(*cb)(int)){(void)cb;}
static inline void lp_gui_on_mouse_move(void(*cb)(int,int,int)){(void)cb;}
static inline void lp_gui_on_mouse_click(void(*cb)(int,int,int)){(void)cb;}
static inline int lp_gui_get_key_state(int k){(void)k;return 0;}
static inline int lp_gui_get_mouse_x(void){return 0;}
static inline int lp_gui_get_mouse_y(void){return 0;}
static inline int lp_gui_set_timer(int h,int ms){(void)h;(void)ms;return 1;}
static inline int lp_gui_set_fps(int h,int fps){(void)h;(void)fps;return 1;}
static inline void lp_gui_run(int h){(void)h;printf("[GUI] Event loop (no-op)\n");}
static inline void lp_gui_run_game(int h){(void)h;}
static inline void lp_gui_message_box(const char*t,const char*m){printf("[%s] %s\n",t?t:"LP",m?m:"");}
static inline int lp_gui_confirm(const char*t,const char*m){printf("[%s] %s (y/n) ",t?t:"LP",m?m:"");return getchar()=='y';}
static inline const char*lp_gui_input_dialog(const char*t,const char*m){(void)t;(void)m;return"";}
static inline const char*lp_gui_file_open_dialog(const char*f){(void)f;return"";}
static inline const char*lp_gui_file_save_dialog(const char*f){(void)f;return"";}
static inline const char*lp_gui_color_picker(void){return"";}
static inline const char*lp_gui_folder_dialog(void){return"";}
/* 2D stubs */
static inline int lp_gui_canvas(int p,int x,int y,int w,int h){(void)p;(void)x;(void)y;(void)w;(void)h;return 1;}
static inline void lp_gui_clear(int c,int color){(void)c;(void)color;}
static inline void lp_gui_present(int c){(void)c;}
static inline void lp_gui_draw_rect(int c,int x,int y,int w,int h,int co){(void)c;(void)x;(void)y;(void)w;(void)h;(void)co;}
static inline void lp_gui_draw_rect_outline(int c,int x,int y,int w,int h,int co,int t){(void)c;(void)x;(void)y;(void)w;(void)h;(void)co;(void)t;}
static inline void lp_gui_draw_circle(int c,int cx,int cy,int r,int co){(void)c;(void)cx;(void)cy;(void)r;(void)co;}
static inline void lp_gui_draw_circle_outline(int c,int cx,int cy,int r,int co,int t){(void)c;(void)cx;(void)cy;(void)r;(void)co;(void)t;}
static inline void lp_gui_draw_line(int c,int x1,int y1,int x2,int y2,int co,int t){(void)c;(void)x1;(void)y1;(void)x2;(void)y2;(void)co;(void)t;}
static inline void lp_gui_draw_text(int c,const char*t,int x,int y,int co,int s){(void)c;(void)t;(void)x;(void)y;(void)co;(void)s;}
static inline void lp_gui_draw_pixel(int c,int x,int y,int co){(void)c;(void)x;(void)y;(void)co;}
static inline void lp_gui_draw_triangle(int c,int x1,int y1,int x2,int y2,int x3,int y3,int co){(void)c;(void)x1;(void)y1;(void)x2;(void)y2;(void)x3;(void)y3;(void)co;}
static inline void lp_gui_draw_image(int c,const char*p,int x,int y,int w,int h){(void)c;(void)p;(void)x;(void)y;(void)w;(void)h;}
/* 3D stubs */
static inline int lp_gui_canvas3d(int p,int x,int y,int w,int h){(void)p;(void)x;(void)y;(void)w;(void)h;return 1;}
static inline int lp_gui_canvasdx11(int p,int x,int y,int w,int h){(void)p;(void)x;(void)y;(void)w;(void)h;return 1;}
static inline void lp_gui_clear3d(int c,double r,double g,double b){(void)c;(void)r;(void)g;(void)b;}
static inline void lp_gui_present3d(int c){(void)c;}
static inline void lp_gui_begin3d(int m){(void)m;}
static inline void lp_gui_end3d(void){}
static inline void lp_gui_vertex3d(double x,double y,double z){(void)x;(void)y;(void)z;}
static inline void lp_gui_color3d(double r,double g,double b){(void)r;(void)g;(void)b;}
static inline void lp_gui_color4d(double r,double g,double b,double a){(void)r;(void)g;(void)b;(void)a;}
static inline void lp_gui_normal3d(double x,double y,double z){(void)x;(void)y;(void)z;}
static inline void lp_gui_texcoord2d(double u,double v){(void)u;(void)v;}
static inline void lp_gui_perspective(double f,double a,double n,double fa){(void)f;(void)a;(void)n;(void)fa;}
static inline void lp_gui_ortho(double l,double r,double b,double t,double n,double f){(void)l;(void)r;(void)b;(void)t;(void)n;(void)f;}
static inline void lp_gui_look_at(double a,double b,double c,double d,double e,double f,double g,double h,double i){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;}
static inline void lp_gui_translate3d(double x,double y,double z){(void)x;(void)y;(void)z;}
static inline void lp_gui_rotate3d(double a,double x,double y,double z){(void)a;(void)x;(void)y;(void)z;}
static inline void lp_gui_scale3d(double x,double y,double z){(void)x;(void)y;(void)z;}
static inline void lp_gui_push_matrix(void){}
static inline void lp_gui_pop_matrix(void){}
static inline void lp_gui_identity(void){}
static inline void lp_gui_mode_projection(void){}
static inline void lp_gui_mode_modelview(void){}
static inline void lp_gui_draw_cube(double x,double y,double z,double s,int c){(void)x;(void)y;(void)z;(void)s;(void)c;}
static inline void lp_gui_draw_sphere(double x,double y,double z,double r,int sl,int st,int c){(void)x;(void)y;(void)z;(void)r;(void)sl;(void)st;(void)c;}
static inline void lp_gui_draw_plane(double y,double s,int c){(void)y;(void)s;(void)c;}
static inline void lp_gui_draw_grid(double y,double s,int d,int c){(void)y;(void)s;(void)d;(void)c;}
static inline void lp_gui_enable_lighting(void){}
static inline void lp_gui_disable_lighting(void){}
static inline void lp_gui_set_light(int id,double x,double y,double z,double r,double g,double b){(void)id;(void)x;(void)y;(void)z;(void)r;(void)g;(void)b;}
static inline void lp_gui_enable_depth_test(void){}
static inline void lp_gui_disable_depth_test(void){}
static inline int lp_gui_load_texture(const char*p){(void)p;return 0;}
static inline void lp_gui_bind_texture(int id){(void)id;}
static inline void lp_gui_unbind_texture(void){}
static inline void lp_gui_prefer_discrete_gpu(void){}
static inline int lp_gui_directx_available(void){return 0;}
static inline int lp_gui_vulkan_available(void){return 0;}
static inline void lp_gui_set_backend(int backend){(void)backend;}
static inline int lp_gui_get_backend(void){return LP_GUI_BACKEND_OPENGL;}
static inline const char*lp_gui_backend_name(void){return"OpenGL";}
static inline const char*lp_gui_backend_info(void){return"backend=OpenGL directx=no vulkan=no";}
static inline const char*lp_gui_gpu_info(void){return"CPU Fallback";}
static inline const char*lp_gui_gpu_vendor(void){return"None";}
static inline const char*lp_gui_gpu_version(void){return"N/A";}
static inline void lp_gui_set_vsync(int on){(void)on;}
static inline void lp_gui_set_msaa(int s){(void)s;}
#endif

#endif /* LP_GUI_H */
