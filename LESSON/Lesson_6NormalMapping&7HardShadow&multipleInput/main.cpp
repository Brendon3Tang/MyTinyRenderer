#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include "our_gl.h"
#include <iostream>
#include <vector>
#include <cmath>
using namespace std;

Model* model = nullptr;
float **shadowBuffer = nullptr;
//float *shadowBuffer = nullptr;
Vec3f light_dir = Vec3f(1., 1., 1.).normalize();   //定义光源的位置（用于shadowing）
Vec3f eye(1, 1, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

const int Height = 800;
const int Width  = 800;
// const char *fileName = "obj/african_head/african_head.obj";
const char *fileName = "obj/diablo3_pose/diablo3_pose.obj";

struct DepthShader : public IShader{
    mat<3,3,float> varyingTri;

    DepthShader() : varyingTri() {}

    //Vec4f vertex(int iface, int nthvert)函数会返回齐次坐标系下的screenCoord。
    virtual Vec4f vertex(int iface, int nthvert){ //vertex Shader
        Vec4f worldCoord = embed<4>(model->vert(iface, nthvert)); //从obj文件中取得vertex的坐标，并在w补上1使其成为齐次坐标。
        Vec4f screenCoord = Viewport * Projection * ModelView * worldCoord; //把worldCoord变成screenCoord
        
        varyingTri.set_col(nthvert, proj<3>(screenCoord/screenCoord.w));
        return screenCoord;
    }

    virtual bool fragment(Vec3f baryCoord, TGAColor & color){
        //float intensity = varyingIntensity * baryCoord; //Gouraud shading中的点P的intensity是根据三个vertex的intensity插值得到的，而非flat shading中根据三角形平面的法线与intensity的值得到
        Vec3f P = varyingTri * baryCoord;   //2X3 matrix * 3X1 vector = 2X1 vector

        color = TGAColor(255,255,255) * (P.z/depth);   //get texture color data

        return false;   //false表示不要忽略这一个pixel
    }
};

struct Shader : public IShader{
    mat<4,4,float> uniform_M;
    mat<4,4,float> uniform_M_invTrans;
    mat<4,4,float> uniform_M_shadow;
    mat<3,3,float> varyingTri;
    mat<2,3,float> varyingUV;

    //constructor
    Shader(Matrix M, Matrix M_invTrans, Matrix M_shadow) : uniform_M(M), uniform_M_invTrans(M_invTrans), uniform_M_shadow(M_shadow), varyingTri(), varyingUV(){}

    //Vec4f vertex(int iface, int nthvert)函数会返回齐次坐标系下的screenCoord。
    virtual Vec4f vertex(int iface, int nthvert){ //vertex Shader
        Vec4f worldCoord = embed<4>(model->vert(iface, nthvert)); //从obj文件中取得vertex的坐标，并在w补上1使其成为齐次坐标。
        Vec4f screenCoord = Viewport * Projection * ModelView * worldCoord; //把worldCoord变成screenCoord
        
        varyingTri.set_col(nthvert, proj<3>(screenCoord/screenCoord.w));    //为每个当前的三角形设置一个矩阵，包含当前三角形的三个顶点投影变换后的坐标(x',y',z')
        varyingUV.set_col(nthvert, model->uv(iface, nthvert));  //为每个当前的三角形设置一个矩阵，包含当前三角形的三个顶点对应的三个纹理坐标(u,v)
        return screenCoord;
    }

    virtual bool fragment(Vec3f baryCoord, TGAColor & color){
        //screenCoordSb stands for screenCoord in shadow buffer
        Vec4f screenCoordSb = uniform_M_shadow * embed<4>(varyingTri * baryCoord);   //通过插值得到P_obj'透视变换后的坐标，并将它变换成以光源为相机位置时P_l'点透视变换后的坐标。
        screenCoordSb = screenCoordSb/screenCoordSb.w;//统一使用仿射坐标来比较,此处得到点P_l'（以光源为相机位置）的透视变换后的仿射坐标
        float shadow = 0.3 + 0.7 * (screenCoordSb.z + 43.34 > shadowBuffer[(int)screenCoordSb.x][(int)screenCoordSb.y]);
        //float shadow = 0.3 + 0.7 * (screenCoordSb.z + 43.34 > shadowBuffer[int(screenCoordSb.x) + int(screenCoordSb.y)*Width]);

        Vec2f uv = varyingUV * baryCoord;   //2X3 matrix * 3X1 vector = 2X1 vector，得到点P的对应的uv纹理坐标
        Vec3f n = proj<3>(uniform_M_invTrans * embed<4>(model->normal(uv))).normalize();    //model->normal()会返回法线贴图中(u,v)对应位置的(r,g,b)
        Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize();
        Vec3f r = (n*(n*l*2.f) - l).normalize();   // reflected light
        Vec3f v = proj<3>(uniform_M * embed<4>(eye)).normalize();

        float diff  = max(0.f, n*l);
        float spec = pow(max(0.f, v * r), model->specular(uv));
        float ambi = 20.f;
        //Vec3f h = (v + l).normalize();
        //float spec = pow(max(0.f, h * n), model->specular(uv));
        
        TGAColor c = model->diffuse(uv);   //get texture color data

        for(int i = 0; i < 3; i++) color[i] = min<float>(ambi +  c[i]*shadow*(1.2f*diff + .6*spec), 255);
        return false;   //false表示不要忽略这一个pixel
    }
};

int main(int argc, char** argv){
    if(argc < 2){
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1;
    }   

    TGAImage zbimage(Width, Height, TGAImage::GRAYSCALE);
    TGAImage image(Width, Height, TGAImage::RGB);

    float **zbuffer = new float*[Width];
    shadowBuffer = new float*[Width];
    for(int i = 0; i < Width; i++){
        zbuffer[i] = new float[Height];
        shadowBuffer[i] = new float[Height];
        for(int j = 0; j < Height; j++){
            zbuffer[i][j] = -std::numeric_limits<float>::max();
            shadowBuffer[i][j] = -std::numeric_limits<float>::max();
        }
    }

    // float *zbuffer = new float[Width*Height];
    // shadowBuffer   = new float[Width*Height];
    // for (int i=Width*Height; --i; ) {
    //     zbuffer[i] = shadowBuffer[i] = -std::numeric_limits<float>::max();
    // }

    for (int m=1; m<argc; m++) {
        model = new Model(argv[m]);
        {//rendering the shadow buffer 
            lookat(light_dir, center, up);
            viewport(Width/8, Height/8, Width*3/4, Height*3/4);
            projection(0);

            DepthShader depthShader;
            Vec4f screenCoord[3];
            for(int i = 0; i < model->nfaces(); i++){
                for(int j = 0; j < 3; j++){
                    screenCoord[j] = depthShader.vertex(i,j);
                }
                triangle(screenCoord, depthShader, zbimage, shadowBuffer);
            }
        }

        Matrix M_shadow = Viewport * Projection * ModelView;

        {
            //draw the model 用项目的方法
            lookat(eye, center, up);
            viewport(Width/8, Height/8, Width*3/4, Height*3/4);
            projection(-1.f/(eye-center).norm());
            
            //用我的方法
            // getModelView(eye, center, up);
            // viewport(Width, Height); 
            // float fovyRad = 40.f * M_PI/180.f;
            // getProjection(fovyRad, Width/Height, 1.0f, 10.0f);
            
            Shader shader(Projection * ModelView, (Projection * ModelView).invert_transpose(), M_shadow * (Viewport*Projection*ModelView).invert());
            for(int i = 0; i < model->nfaces(); i++){  
                Vec4f screenCoords[3];
                for(int j = 0; j < 3; j++){
                    screenCoords[j] = shader.vertex(i,j);
                }
                triangle(screenCoords, shader, image, zbuffer);
            }
        }
        delete model;
    }

    zbimage.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    zbimage.write_tga_file("./output/L7_zbuffer3.tga");
    
    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("./output/L7_outputShadow3.tga");
    delete []shadowBuffer;
    delete []zbuffer;
    return 0;
}
