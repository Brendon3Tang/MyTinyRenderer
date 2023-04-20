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

    Viewport[0][0] = w/2.f;
    Viewport[1][1] = h/2.f;
    Viewport[2][2] = depth/2.f;
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
// barycentric会用重心坐标判断点P是否在由pts0,pts1,pts2三个顶点组成的三角形内，如果是就返回true，否则返回false
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

void triangle(Vec4f *pts, /*Vec2f *uv,*/ IShader &shader, TGAImage &image, float **zbuffer){
    //对于每个三角形，我们会在屏幕内找到最小的能够把它包裹起来的box，然后在这个box内部进行shading（提高效率）
    Vec2f miniBound(image.get_width()-1, image.get_height()-1);
    Vec2f maxBound(0,0);

    //遍历三角形的三个顶点，找到每个三角形的最高点与最低点，最左点与最右点，这个三角形一定在这个box内
    for(int i = 0; i < 3; i++){
        miniBound.x = max(0.f, min(miniBound.x, pts[i].x/pts[i].w));
        miniBound.y = max(0.f, min(miniBound.y, pts[i].y/pts[i].w));

        maxBound.x = min((float)image.get_width()-1, max(maxBound.x, pts[i].x/pts[i].w));
        maxBound.y = min((float)image.get_height()-1, max(maxBound.y, pts[i].y/pts[i].w));
    }

    Vec4f P;
    TGAColor color;
    for (int i=miniBound.x; i <= maxBound.x; i++) {
        for (int j =miniBound.y; j <=maxBound.y; j++) {
            P = Vec4f(i, j, 0, 0); //取得当前点P的
            Vec2f projPts[3] = {proj<2>(pts[0]/pts[0].w), proj<2>(pts[1]/pts[1].w), proj<2>(pts[2]/pts[2].w)};  //把每个顶点的齐次坐标反齐次化然后投射回2D，只保留x，y
            Vec3f baryCoord = barycentric(projPts, proj<2>(P));//用三个顶点和点P求点P的重心坐标
            if(baryCoord.x >= 0.0f && baryCoord.y >= 0.f && baryCoord.z >= 0.f){
                //插值求点P的z值，用于深度测试。
                for(int k = 0; k < 3; k++){
                    P.z += pts[k].z * baryCoord[k];
                    P.w += pts[k].w * baryCoord[k];
                }
                
                // 注意z不一定需要反齐次化，两种情况都可以，只是出来的buffer颜色不同
                if (zbuffer[(int)P.x][(int)P.y] < int(P.z/P.w) /*int(P.z/P.w + 0.5f)*/) {
                    zbuffer[(int)P.x][(int)P.y] = int(P.z/P.w) /*int(P.z/P.w + 0.5f)*/;
                // if (zbuffer[(int)P.x+(int)P.y*image.get_width()] < int(P.z/P.w) /*int(P.z/P.w + 0.5f)*/) {
                //     zbuffer[(int)P.x+(int)P.y*image.get_width()] = int(P.z/P.w) /*int(P.z/P.w + 0.5f)*/;
                    bool discard = shader.fragment(baryCoord, color);
                    if (!discard) {
                        image.set(P.x, P.y, color);
                    }
                }
            }
        }
    }
}

// void triangle(mat<4,3,float> &clipc, IShader &shader, TGAImage &image, float *zbuffer) {
//     mat<3,4,float> pts  = (Viewport*clipc).transpose(); // transposed to ease access to each of the points
//     mat<3,2,float> pts2;
//     for (int i=0; i<3; i++) pts2[i] = proj<2>(pts[i]/pts[i][3]);

//     Vec2f bboxmin( std::numeric_limits<float>::max(),  std::numeric_limits<float>::max());
//     Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
//     Vec2f clamp(image.get_width()-1, image.get_height()-1);
//     for (int i=0; i<3; i++) {
//         for (int j=0; j<2; j++) {
//             bboxmin[j] = std::max(0.f,      std::min(bboxmin[j], pts2[i][j]));
//             bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts2[i][j]));
//         }
//     }
//     Vec2i P;
//     TGAColor color;
//     for (P.x=bboxmin.x; P.x<=bboxmax.x; P.x++) {
//         for (P.y=bboxmin.y; P.y<=bboxmax.y; P.y++) {
//             Vec2f projP[3] = {pts2[0], pts2[1], pts2[2]};
//             Vec3f bc_screen  = barycentric(projP, P);
//             Vec3f bc_clip    = Vec3f(bc_screen.x/pts[0][3], bc_screen.y/pts[1][3], bc_screen.z/pts[2][3]);
//             bc_clip = bc_clip/(bc_clip.x+bc_clip.y+bc_clip.z);
//             float frag_depth = clipc[2]*bc_clip;
//             if (bc_screen.x<0 || bc_screen.y<0 || bc_screen.z<0 || zbuffer[P.x+P.y*image.get_width()]>frag_depth) continue;
//             bool discard = shader.fragment(bc_clip, color);
//             if (!discard) {
//                 zbuffer[P.x+P.y*image.get_width()] = frag_depth;
//                 image.set(P.x, P.y, color);
//             }
//         }
//     }
// }