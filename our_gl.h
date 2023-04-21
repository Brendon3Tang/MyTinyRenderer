#ifndef __OUR_GL_H__
#define __OUR_GL_H__

#include "geometry.h"
#include "tgaimage.h"

extern Matrix ModelView;
extern Matrix Projection;
extern Matrix Viewport;
const float depth = 2000.f;

void viewport(int x, int y, int w, int h);
void viewport(int x, int y);  //
void lookat(Vec3f eye, Vec3f center, Vec3f up);
void getModelView(Vec3f eye, Vec3f center, Vec3f up);
void getProjection(float fov, float aspect, float nearDist, float farDist);
void projection(float coeff=0.f); // coeff = -1/c

struct IShader {
    virtual ~IShader();
    virtual Vec4f vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f bcScreen, Vec3f bcClip, TGAColor &color) = 0;
};

void triangle(Vec4f *pts, IShader &shader, TGAImage &image, float **zbuffer);
#endif //__OUR_GL_H__
