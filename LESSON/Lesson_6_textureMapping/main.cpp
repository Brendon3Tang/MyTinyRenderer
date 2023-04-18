#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include "our_gl.h"
#include <iostream>
#include <vector>
#include <cmath>
using namespace std;

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor green = TGAColor(0,   255, 128, 255);

Model* model = nullptr;
Vec3f light_dir = Vec3f(1., -1., 1.).normalize();   //定义光源的位置（用于shadowing）
Vec3f eye(1, 1, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

const int Height = 800;
const int Width  = 800;
const char *fileName = "obj/african_head.obj";

struct GouraudShader : public IShader{
    Vec3f varyingIntensity;
    mat<2,3,float> varyingUV;

    //Vec4f vertex(int iface, int nthvert)函数会返回齐次坐标系下的screenCoord。
    virtual Vec4f vertex(int iface, int nthvert){ //vertex Shader
        Vec4f worldCoord = embed<4>(model->vert(iface, nthvert)); //从obj文件中取得vertex的坐标，并在w补上1使其成为齐次坐标。
        Vec4f screenCoord = Viewport * Projection * ModelView * worldCoord; //把worldCoord变成screenCoord
        
        varyingIntensity[nthvert] = model->normal(iface, nthvert) * light_dir;
        varyingUV.set_col(nthvert, model->uv(iface, nthvert));
        return screenCoord;
    }

    virtual bool fragment(Vec3f baryCoord, TGAColor & color){
        float intensity = varyingIntensity * baryCoord; //Gouraud shading中的点P的intensity是根据三个vertex的intensity插值得到的，而非flat shading中根据三角形平面的法线与intensity的值得到
        Vec2f uv = varyingUV * baryCoord;   //2X3 matrix * 3X1 vector = 2X1 vector
        
        color = model->diffuse(uv)*intensity;   //get texture color data

        return false;   //false表示不要忽略这一个pixel
    }
};

int main(int argc, char** argv){
    TGAImage image(Width, Height, TGAImage::RGB);
    TGAImage zbimage(Width, Height, TGAImage::GRAYSCALE);

    if(argc == 2)
        model = new Model(argv[1]);
    else
        model = new Model(fileName);
    
    //设置zBuffer
    float **zbuffer = new float*[Width];
    for(int i = 0; i < Width; i++){
        zbuffer[i] = new float[Height];
        for(int j = 0; j < Height; j++){
            zbuffer[i][j] = -std::numeric_limits<float>::max();
        }
    }

    //draw the model 用项目的方法
    lookat(eye, center, up);
    viewport(Width/8, Height/8, Width*3/4, Height*3/4);
    projection(-1.f/(eye-center).norm());
    
    //用我的方法
    // getModelView(eye, center, up);
    // viewport(Width, Height); 
    // float fovyRad = 40.f * M_PI/180.f;
    // getProjection(fovyRad, Width/Height, 1.0f, 10.0f);

    GouraudShader shader;
    for(int i = 0; i < model->nfaces(); i++){  
        Vec4f screenCoords[3];
        for(int j = 0; j < 3; j++){
            screenCoords[j] = shader.vertex(i,j);
        }
        triangle(screenCoords, shader, image, zbuffer);
    }

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("./output/L6_outputTexture.tga");


    
    for (int i=0; i<Width; i++) {
        for (int j=0; j<Height; j++) {
            zbimage.set(i, j, TGAColor(zbuffer[i][j]));
        }
    }
    zbimage.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    zbimage.write_tga_file("./output/L6_zbuffer.tga");
    delete model;
    delete []zbuffer;
    return 0;
}
