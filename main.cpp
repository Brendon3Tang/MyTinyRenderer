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
Vec3f light_dir = Vec3f(1., 1., 1.);   //定义光源的位置（用于shadowing）
Vec3f eye(0, 1, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

const int Height = 800;
const int Width  = 800;

struct DepthShader : public IShader{
    mat<3,3,float> varyingTri;

    DepthShader() : varyingTri() {}

    //Vec4f vertex(int iface, int nthvert)函数会返回齐次坐标系下的screenCoord。
    virtual Vec4f vertex(int iface, int nthvert){ //vertex Shader
        Vec4f worldCoord = embed<4>(model->vert(iface, nthvert)); //从obj文件中取得vertex的坐标，并在w补上1使其成为齐次坐标。
        Vec4f screenCoord = Projection * ModelView * worldCoord; //把worldCoord变成screenCoord
        Vec4f screenCoordCopy = Viewport * screenCoord;
        varyingTri.set_col(nthvert, proj<3>(screenCoordCopy/screenCoordCopy.w));
        return screenCoord;
    }

    virtual bool fragment(Vec3f bcScreen, Vec3f bcClip, TGAColor & color){
        Vec3f P = varyingTri * bcScreen;   //2X3 matrix * 3X1 vector = 2X1 vector
        
        color = TGAColor(255,255,255) * (P.z/depth);   //get texture color data 
        return false;   //false表示不要忽略这一个pixel
    }
};

struct GouraudShader : public IShader{
    Vec3f varyingIntensity;
    mat<2,3,float> varyingUV;

    //Vec4f vertex(int iface, int nthvert)函数会返回齐次坐标系下的screenCoord。
    virtual Vec4f vertex(int iface, int nthvert){ //vertex Shader
        Vec4f worldCoord = embed<4>(model->vert(iface, nthvert)); //从obj文件中取得vertex的坐标，并在w补上1使其成为齐次坐标。
        Vec4f screenCoord = Projection * ModelView * worldCoord; //把worldCoord变成screenCoord
        
        varyingIntensity[nthvert] = model->normal(iface, nthvert) * light_dir;
        varyingUV.set_col(nthvert, model->uv(iface, nthvert));
        return screenCoord;
    }

    virtual bool fragment(Vec3f bcScreen, Vec3f bcClip, TGAColor & color){
        float intensity = varyingIntensity * bcClip; //Gouraud shading中的点P的intensity是根据三个vertex的intensity插值得到的，而非flat shading中根据三角形平面的法线与intensity的值得到
        Vec2f uv = varyingUV * bcClip;   //2X3 matrix * 3X1 vector = 2X1 vector
        
        color = model->diffuse(uv)*intensity;   //get texture color data

        return false;   //false表示不要忽略这一个pixel
    }
};

struct NormalShader : public IShader{
    mat<4,4,float> uniform_M;
    mat<4,4,float> uniform_M_invTrans;
    mat<4,4,float> uniform_M_shadow;
    mat<3,3,float> varyingTri;
    mat<2,3,float> varyingUV;

    //test
    mat<3,3,float> view_tri;    // triangle in view coordinates
    mat<3,3,float> varying_nrm; // normal per vertex to be interpolated by FS
    //mat<4,3,float> varying_tri; // triangle coordinates (clip coordinates), written by VS, read by FS
    //test end

    //constructor
    NormalShader(Matrix M, Matrix M_invTrans, Matrix M_shadow) : uniform_M(M), uniform_M_invTrans(M_invTrans), uniform_M_shadow(M_shadow), varyingTri(), varyingUV(){}

    //Vec4f vertex(int iface, int nthvert)函数会返回齐次坐标系下的screenCoord。
    virtual Vec4f vertex(int iface, int nthvert){ //vertex Shader
        Vec4f worldCoord = embed<4>(model->vert(iface, nthvert)); //从obj文件中取得vertex的坐标，并在w补上1使其成为齐次坐标。
        Vec4f screenCoord = ModelView * worldCoord; //把worldCoord变成screenCoord
        
        varyingUV.set_col(nthvert, model->uv(iface, nthvert));  //为每个当前的三角形设置一个矩阵，包含当前三角形的三个顶点对应的三个纹理坐标(u,v)
        view_tri.set_col(nthvert, proj<3>(screenCoord));
        
        screenCoord = Projection * screenCoord;
        Vec4f screenCoordCopy = Viewport * screenCoord;
        varyingTri.set_col(nthvert, proj<3>(screenCoordCopy / screenCoordCopy.w));    //为每个当前的三角形设置一个矩阵，包含当前三角形的三个顶点的坐标(x,y,z)
        varying_nrm.set_col(nthvert, proj<3>(uniform_M_invTrans*embed<4>(model->normal(iface, nthvert), 0.f)));
        return screenCoord;
    }

    virtual bool fragment(Vec3f bcScreen, Vec3f bcClip, TGAColor & color){
        Vec4f pPrime = uniform_M_shadow * embed<4>(varyingTri * bcScreen);
        pPrime = pPrime/pPrime.w;
        
        pPrime.x = min(Width-1, (int)pPrime.x);
        pPrime.x = max(0, (int)pPrime.x);
        pPrime.y = min(Height-1, (int)pPrime.y);
        pPrime.y = max(0, (int)pPrime.y);
        float fragment_depth = pPrime.z;
        float shadow = 0.3 + 0.7 * (fragment_depth + 43.34 > shadowBuffer[(int)pPrime.x][(int)pPrime.y]);

        Vec3f bn = (varying_nrm*bcClip).normalize();
        Vec2f uv = varyingUV*bcClip;

        mat<3,3,float> A;
        A[0] = view_tri.col(1) - view_tri.col(0);
        A[1] = view_tri.col(2) - view_tri.col(0);
        A[2] = bn;
        mat<3,3,float> A_invert = A.invert();
        Vec3f bu = A_invert*Vec3f(varyingUV[0][1]-varyingUV[0][0], varyingUV[0][2]-varyingUV[0][0], 0);
        Vec3f bv = A_invert*Vec3f(varyingUV[1][1]-varyingUV[1][0], varyingUV[1][2]-varyingUV[1][0], 0);
        mat<3,3,float> B;
        B.set_col(0, bu.normalize());
        B.set_col(1, bv.normalize());
        B.set_col(2, bn);

        Vec3f n = (B*model->normal(uv)).normalize();
        Vec3f l = proj<3>(uniform_M * embed<4>(light_dir, 0.f)).normalize();
        Vec3f r = (n*(n*l)*2 - l).normalize(); 
        Vec3f v = proj<3>(uniform_M * embed<4>(eye)).normalize();

        float diff = std::max(0.f, n*l);
        float spec = pow(max(v * r, 0.f), model->specular(uv));
        TGAColor c = model->diffuse(uv);
        for(int i = 0; i < 3; i++) color[i] = min<float>(10 +  c[i]*shadow*(1.2*diff + 0.6*spec), 255);
        return false;
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
    
    for (int m=1; m<argc; m++) {
         model = new Model(argv[m]);
        {//rendering the shadow buffer 
            lookat(light_dir, center, up);
            viewport(Width/8, Height/8, Width*3/4, Height*3/4);
            projection(0);
            
            //用我的办法
            // getModelView(light_dir, center, up);
            // viewport(Width, Height); 
            // float fovyRad = 40.f * M_PI/180.f;
            // getProjection(fovyRad, Width/Height, 1.0f, 10.0f);

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
            

            NormalShader normalShader(Projection * ModelView, (Projection * ModelView).invert_transpose(), M_shadow * (Viewport*Projection*ModelView).invert());
            for(int i = 0; i < model->nfaces(); i++){  
                Vec4f screenCoords[3];
                for(int j = 0; j < 3; j++){
                    screenCoords[j] = normalShader.vertex(i,j);
                }
                triangle(screenCoords, normalShader, image, zbuffer);
            }
        }
        delete model;
    }

    zbimage.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    zbimage.write_tga_file("./output/L7_zbuffer2-b.tga");
    
    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("./output/L7_outputShadow2-b.tga");
    delete []shadowBuffer;
    delete []zbuffer;
    return 0;
}