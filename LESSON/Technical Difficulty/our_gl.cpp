#include "our_gl.h"
#include <cmath>
#include <iostream>
using namespace std;

Matrix ModelView;
Matrix Viewport;
Matrix Projection;

IShader::~IShader() {}

void viewport(int x, int y, int w, int h){
    Viewport = Matrix::identity();
    Viewport[0][3] = x+w/2.f;
    Viewport[1][3] = y+h/2.f;
    Viewport[2][3] = depth/2.f;
    //Viewport[2][3] = 1.f;

    Viewport[0][0] = w/2.f;
    Viewport[1][1] = h/2.f;
    Viewport[2][2] = depth/2.f;
    //Viewport[2][2] = 0.f;
}

//101 formula
void viewport(int x, int y){
    Viewport = Matrix::identity();
    Viewport[0][3] = x/2.f;
    Viewport[1][3] = y/2.f;
    Viewport[2][3] = depth/2.f;

    Viewport[0][0] = x/2.f;
    Viewport[1][1] = y/2.f;
    Viewport[2][2] = depth/2.f;
}

void lookat(Vec3f eye, Vec3f center, Vec3f up){
    Vec3f z = (eye - center).normalize();
    Vec3f x = cross(up, z).normalize();
    Vec3f y = cross(z, x).normalize();
    ModelView = Matrix::identity();
    for(int i = 0; i < 3; i++){
        ModelView[0][i] = x[i];
        ModelView[1][i] = y[i];
        ModelView[2][i] = z[i];
        ModelView[i][3] = -center[i];
    }
}

void getModelView(Vec3f eye, Vec3f center, Vec3f up){
    Vec3f z = (eye - center).normalize();
    Vec3f x = cross(up, z).normalize();
    Vec3f y = cross(z, x).normalize();
    ModelView = Matrix::identity();
    for(int i = 0; i < 3; i++){
        ModelView[i][0] = x[i];
        ModelView[i][1] = y[i];
        ModelView[i][2] = z[i];
        ModelView[i][3] = eye[i];
    }
    ModelView[3][3] = 1.f;
    // return C.inverse();
    ModelView = ModelView.invert();
}

//formula from: https://stackoverflow.com/questions/18404890/how-to-build-perspective-projection-matrix-no-api
void getProjection(float fov, float aspect, float nearDist, float farDist)
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

    Projection = Matrix::identity();

    float frustumDepth = farDist - nearDist;
    float oneOverDepth = 1 / frustumDepth;

    Projection[0][0] = 1 / (tan(0.5f * fov) * aspect);
    Projection[1][1] = 1 / tan(0.5f * fov);
    Projection[2][2] = farDist * oneOverDepth;
    Projection[3][2] = (-farDist * nearDist) * oneOverDepth;
    Projection[2][3] = 1;
    Projection[3][3] = 0;
}

void projection(float coeff){
    Projection = Matrix::identity();
    Projection[3][2] = coeff;
}

//new method: 用 barycentric画:
//barycentric会用重心坐标判断点P是否在由pts0,pts1,pts2三个顶点组成的三角形内，如果是就返回true，否则返回false
Vec3f barycentric(Vec2f *pts, Vec2f P){
    Vec2f A = pts[0];
    Vec2f B = pts[1];
    Vec2f C = pts[2];
    float alpha = (-((float)P.x - B.x)*(C.y - B.y) + (P.y - B.y)*(C.x - B.x)) / 
    (-(A.x - B.x)*(C.y - B.y) + (A.y - B.y)*(C.x - B.x));

    float beta = (-((float)P.x - C.x)*(A.y - C.y) + (P.y - C.y)*(A.x - C.x)) / 
    (-(B.x - C.x)*(A.y - C.y) + (B.y - C.y)*(A.x - C.x));

    float gamma = 1.0f - alpha - beta;

    return Vec3f(alpha, beta, gamma);
}

void triangle(Vec4f *pts, IShader &shader, TGAImage &image, float **zbuffer){
    Vec4f ptsPrimeHomo[3] = {Viewport * pts[0], Viewport * pts[1], Viewport * pts[2]};  //经过MVP变换后三个顶点的screenCoord，齐次坐标系
    Vec2f ptsPrimeAffine[3] = {proj<2>(ptsPrimeHomo[0]/ptsPrimeHomo[0].w), proj<2>(ptsPrimeHomo[1]/ptsPrimeHomo[1].w), proj<2>(ptsPrimeHomo[2]/ptsPrimeHomo[2].w)};   //得到MVP后三个顶点的screenCoord，仿射坐标系

    Vec2f miniBound(image.get_width()-1, image.get_height()-1);
    Vec2f maxBound(0,0);

    //遍历三角形的三个顶点，找到每个三角形的最高点与最低点，最左点与最右点，这个三角形一定在这个box内
    for(int i = 0; i < 3; i++){
        miniBound.x = max(0.f, min(miniBound.x, ptsPrimeAffine[i].x));
        miniBound.y = max(0.f, min(miniBound.y, ptsPrimeAffine[i].y));

        maxBound.x = min((float)image.get_width()-1, max(maxBound.x, ptsPrimeAffine[i].x));
        maxBound.y = min((float)image.get_height()-1, max(maxBound.y, ptsPrimeAffine[i].y));
    }

    Vec2i scP;  //scP stands for screenCoords of point P ( = P after MVP = P')，由于baryCentric只需要(x,y)，所以我们只用Vec2i
    TGAColor color;
    for (int i=miniBound.x; i <= maxBound.x; i++) {
        for (int j =miniBound.y; j <=maxBound.y; j++) {
            scP = Vec2i(i, j); //取得当前点P’的经过MVP后的仿射坐标（因为i ， j是根据pts得到的）
            Vec2f projPts2[3] = {ptsPrimeAffine[0], ptsPrimeAffine[1], ptsPrimeAffine[2]};  //把经过MVP后的每个顶点的仿射坐标x，y传入baryCentric
            Vec3f bcScreen = barycentric(projPts2, scP);//用经过MVP后的三个顶点和点P'求点P‘的重心坐标
            Vec3f bcClip = Vec3f(bcScreen.x/pts[0].w, bcScreen.y/pts[1].w, bcScreen.z/pts[2].w);
            bcClip = bcClip/(bcClip.x+bcClip.y+bcClip.z);   //反变换后，得到点P（未经过MVP变换）的重心坐标

            float fragment_z = 0.f;
            for(int k = 0; k < 3; k++){
                fragment_z += pts[k].z * bcClip[k];
            }
            // cout << "zbuffer: " << zbuffer[(int)P.x][(int)P.y] << endl; 
            if(bcScreen.x < 0.0f || bcScreen.y < 0.f || bcScreen.z < 0.f || zbuffer[scP.x][scP.y] > fragment_z) continue;
            // cout << "p.Z: " << P.z << endl; 
            // cout << "running" << endl;
            bool discard = shader.fragment(bcClip, color);
            if (!discard) {
                zbuffer[scP.x][scP.y] = fragment_z;
                image.set(scP.x, scP.y, color);
            }
        }
    }
}