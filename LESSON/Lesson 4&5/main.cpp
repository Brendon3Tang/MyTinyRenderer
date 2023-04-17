/**
 * Bresenham’s Line Drawing Algorithm
*/
#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
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

const int Height = 800;
const int Width  = 800;
const int Depth  = 255;
const char *fileName = "obj/african_head.obj";

Matrix lookat(Vec3f eye, Vec3f center, Vec3f up){
    Vec3f z = (eye - center).normalize();
    Vec3f x = (up ^ z).normalize();
    Vec3f y = (z ^ x).normalize();
    Matrix C = Matrix::identity(4);
    for(int i = 0; i < 3; i++){
        C[0][i] = x[i];
        C[1][i] = y[i];
        C[2][i] = z[i];
        C[i][3] = -center[i];
    }
    return C;
}

Matrix getModelView(Vec3f eye, Vec3f center, Vec3f up){
    Vec3f z = (eye - center).normalize();
    Vec3f x = (up ^ z).normalize();
    Vec3f y = (z ^ x).normalize();
    Matrix C = Matrix::identity(4);
    for(int i = 0; i < 3; i++){
        C[i][0] = x[i];
        C[i][1] = y[i];
        C[i][2] = z[i];
        C[i][3] = eye[i];
    }
    return C.inverse();
}

Matrix viewport(int x, int y, int w, int h){
    Matrix m = Matrix::identity(4);
    m[0][3] = x+w/2.f;
    m[1][3] = y+h/2.f;
    m[2][3] = Depth/2.f;

    m[0][0] = w/2.f;
    m[1][1] = h/2.f;
    m[2][2] = Depth/2.f;
    return m;
}

//101 formula
Matrix viewport(int x, int y){
    Matrix m = Matrix::identity(4);
    m[0][3] = x/2.f;
    m[1][3] = y/2.f;
    m[2][3] = Depth/2.f;

    m[0][0] = x/2.f;
    m[1][1] = y/2.f;
    m[2][2] = Depth/2.f;
    return m;
}

//formula from: https://stackoverflow.com/questions/18404890/how-to-build-perspective-projection-matrix-no-api
Matrix getProjection(float fov, float aspect, float nearDist, float farDist, bool leftHanded /* = true */ )
{
    //
    // General form of the Projection Matrix
    //
    // uh = Cot( fov/2 ) == 1/Tan(fov/2)
    // uw / uh = 1/aspect
    // 
    //   uw         0       0       0
    //    0        uh       0       0
    //    0         0      f/(f-n)  1
    //    0         0    -fn/(f-n)  0
    //
    // Make result to be identity first

    // check for bad parameters to avoid divide by zero:
    // if found, assert and return an identity matrix.

    Matrix Perspective = Matrix::identity(4);

    float frustumDepth = farDist - nearDist;
    float oneOverDepth = 1 / frustumDepth;

    Perspective[0][0] = 1 / (tan(0.5f * fov) * aspect);
    Perspective[1][1] = 1 / tan(0.5f * fov);
    Perspective[2][2] = farDist * oneOverDepth;
    Perspective[3][2] = (-farDist * nearDist) * oneOverDepth;
    Perspective[2][3] = 1;
    Perspective[3][3] = 0;
    
    return Perspective;
}

//new method: 用 barycentric画:
// barycentric会用重心坐标判断点P是否在由pts0,pts1,pts2三个顶点组成的三角形内，如果是就返回true，否则返回false
Vec3f barycentric(Vec3f *pts, Vec3f P){
    Vec3f A = pts[0];
    Vec3f B = pts[1];
    Vec3f C = pts[2];
    float alpha = (-((float)P.x - B.x)*(C.y - B.y) + (P.y - B.y)*(C.x - B.x)) / 
    (-(A.x - B.x)*(C.y - B.y) + (A.y - B.y)*(C.x - B.x));

    float beta = (-((float)P.x - C.x)*(A.y - C.y) + (P.y - C.y)*(A.x - C.x)) / 
    (-(B.x - C.x)*(A.y - C.y) + (B.y - C.y)*(A.x - C.x));

    float gamma = 1.0f - alpha - beta;

    return Vec3f(alpha, beta, gamma);
}

// Lesson 4 三角形渲染 Gouraud Shading
void triangle(Vec3f *pts, Vec2i *uv, float **zbuffer, TGAImage &image, float *intensity){
    //对于每个三角形，我们会在屏幕内找到最小的能够把它包裹起来的box，然后在这个box内部进行shading（提高效率）
    Vec2f miniBound(image.get_width()-1, image.get_height()-1);
    Vec2f maxBound(0,0);

    //遍历三角形的三个顶点，找到每个三角形的最高点与最低点，最左点与最右点，这个三角形一定在这个box内
    for(int i = 0; i < 3; i++){
        miniBound.x = max(0.f, min(miniBound.x, pts[i].x));
        miniBound.y = max(0.f, min(miniBound.y, pts[i].y));

        maxBound.x = min((float)image.get_width()-1, max(maxBound.x, pts[i].x));
        maxBound.y = min((float)image.get_height()-1, max(maxBound.y, pts[i].y));
    }

    //定义点P。P可以是box内的任意一个点，我们需要逐个遍历，如果P刚好在三角形内部，那么我们就把这个三角形着色shading
    Vec3f P;
    for(int i = miniBound.x; i <= maxBound.x; i++){
        for(int j = miniBound.y; j <= maxBound.y; j++){
            P = Vec3f(i, j, 0);    //取得当前点P
            Vec2i uvP;  //uvP记录当前点P的插值后的对应的纹理坐标
            Vec3f baryCentricCoord = barycentric(pts, P);
            float intensityP = 0.f;  //IntensityP记录了P点插值后的intensity
            //求出重心坐标alpha, beta, lambda。当且仅当这三个值都大于0时，点才会在三角形平面内
            if(baryCentricCoord.x >= 0.0f && baryCentricCoord.y >= 0.f && baryCentricCoord.z >= 0.f){  //如果当前点P在三角形内，就shading
                for(int k = 0; k < 3; k++){
                    P.z += pts[k].z * baryCentricCoord[k];   //P.z的插值就是三角形三个顶点ABC的z值的加权平均，即A.z * Alpha(顶点A) + B.z * Beta（顶点B） + C.z * Gamma(顶点C)
                    uvP.x += uv[k].x * baryCentricCoord[k]; //同理，插值后P的纹理坐标uvP.x的值就是三角形三个顶点对应的纹理坐标的值的加权平均，
                    uvP.y += uv[k].y * baryCentricCoord[k]; // 插值后P的纹理坐标y同理
                    intensityP += intensity[k] * baryCentricCoord[k];
                }
                if (zbuffer[(int)P.x][(int)P.y]<P.z) {
                    zbuffer[(int)P.x][(int)P.y] = P.z;

                    TGAColor color = model->diffuse(uvP);
                    image.set(P.x, P.y, TGAColor(color.bgra[2], color.bgra[1], color.bgra[0])*intensityP);
                }
            }
        }
    }
}

int main(int argc, char** argv){
    //flat shader + simple shadow
    TGAImage image(Width, Height, TGAImage::RGB);
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
    // Matrix ModelView = lookat(eye, center, Vec3f(0, 1, 0));
    // Matrix ViewPort = viewport(Width/8, Height/8, Width*3/4, Height*3/4);
    // Matrix Projection = Matrix::identity(4);
    // Projection[3][2] = -1.f/(eye-center).norm();
    
    //用我的方法
    Matrix ModelView = getModelView(eye, center, Vec3f(0, 1, 0));
    Matrix ViewPort = viewport(Width, Height);
    float fovyRad = 40.f * M_PI/180.f;
    Matrix Projection = getProjection(fovyRad, Width/Height, 1.0f, 10.0f, true);
    
    //遍历所有的面，每个face都是一个三角形，有三个顶点vertex，每个vertex有(x,y,z)三个坐标
    for(int i = 0; i < model->nfaces(); i++){
        vector<int> face = model->face(i); //用vector取出每个face，方便后续操作
        Vec3f screenCoord[3];   //保存三角形三个vertex在屏幕上出现的位置
        Vec3f worldCoord[3];    //保存三角形三个vertex的实际坐标
        float intensity[3];     ////保存三角形三个vertex的intensity
        //遍历三个顶点
        for(int j = 0; j < 3; j++){
            worldCoord[j] = model->vert(face[j]);   //用vert()取出该vertex的坐标(x,y)并保存在worldCoord中
            screenCoord[j] = Vec3f(ViewPort*Projection*ModelView*Matrix(worldCoord[j]));    //转换到屏幕上的位置
            intensity[j] = model->norm(i, j) * light_dir;   //我们要计算每一个vertex的intensity，而不是每一个三角形平面的intensity
        }
        // // 求出三角形平面的法线。 Gouraud Shading中不再需要，因为我么需要的是每个顶点的法线，而非每个平面的法线
        // Vec3f n = Vec3f(worldCoord[2] - worldCoord[0]) ^ Vec3f(worldCoord[1] - worldCoord[0]);
        // n = n.normalize();
        
        // 穿过三角形平面的光的intensity与光源和平面法线的夹脚有关，当光来源与平面平行时，基本没有光穿过平面，可以不着色（阴影）。因此我们此处用点乘
        // if(intensity > 0){   //只有有光穿过平面时我们才会着色，且颜色深度与intensity有关。
        Vec2i uv[3];
        for(int j = 0; j < 3; j++)  uv[j] = model->uv(i, j);//取得第i个面第j个顶点的uv坐标
        //triangle(screenCoord[0], screenCoord[1], screenCoord[2], uv[0], uv[1], uv[2], image, intensity, zbuffer);
        triangle(screenCoord, uv, zbuffer, image, intensity);
        //}

    }

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("./output/L5_outputTexture2.tga");


    TGAImage zbimage(Width, Height, TGAImage::GRAYSCALE);
    for (int i=0; i<Width; i++) {
        for (int j=0; j<Height; j++) {
            zbimage.set(i, j, TGAColor(zbuffer[i][j]));
        }
    }
    zbimage.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    zbimage.write_tga_file("./output/L5_zbuffer2.tga");
    delete model;
    delete []zbuffer;
    return 0;
}