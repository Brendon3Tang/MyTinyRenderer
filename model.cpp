// #include <iostream>
// #include <string>
// #include <fstream>
// #include <sstream>
// #include <vector>
// #include "model.h"

// Model::Model(const char *filename) : verts_(), faces_() {
//     std::ifstream in;
//     in.open (filename, std::ifstream::in);
//     if (in.fail()) return;
//     std::string line;
//     while (!in.eof()) {
//         std::getline(in, line);
//         std::istringstream iss(line.c_str());
//         char trash;
//         if (!line.compare(0, 2, "v ")) {
//             iss >> trash;
//             Vec3f v;
//             for (int i=0;i<3;i++) iss >> v[i];
//             verts_.push_back(v);
//         } else if (!line.compare(0, 2, "f ")) {
//             std::vector<int> f;
//             int itrash, idx;
//             iss >> trash;
//             while (iss >> idx >> trash >> itrash >> trash >> itrash) {
//                 idx--; // in wavefront obj all indices start at 1, not zero
//                 f.push_back(idx);
//             }
//             faces_.push_back(f);
//         }
//     }
//     std::cerr << "# v# " << verts_.size() << " f# "  << faces_.size() << std::endl;
// }

// Model::~Model() {
// }

// int Model::nverts() {
//     return (int)verts_.size();
// }

// int Model::nfaces() {
//     return (int)faces_.size();
// }

// std::vector<int> Model::face(int idx) {
//     return faces_[idx];
// }

// Vec3f Model::vert(int i) {
//     return verts_[i];
// }


#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char *filename) : verts_(), faces_(), norms_(), uv_() {
    std::ifstream in;
    in.open (filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (!in.eof()) {
        std::getline(in, line); //把file里的data全部放入string line里。
        std::istringstream iss(line.c_str());   //line.c_str()它返回当前字符串的首字符地址。
                                                //换种说法，c_str()函数返回一个指向正规C字符串的指针常量，
                                                //内容与本string串相同。这是为了与C语言兼容，在C语言中没有string类型，
                                                //故必须通过string类对象的成员函数c_str()把string对象转换成C中的字符串样式。
        char trash;
        if (!line.compare(0, 2, "v ")) {    //compare(line的起始位置start，line中要被比较的子字符串的长度 i:[start, i-1], 与line比较的字符)
            iss >> trash;                   //compare 返回0如果相等。
            Vec3f v;
            for (int i=0;i<3;i++) iss >> v[i];  // 每个顶点v 由（x,y,z）三个元素组成。
            verts_.push_back(v);
        } else if (!line.compare(0, 3, "vn ")) {
            iss >> trash >> trash;  //把'v','n'先放入trash，然后再把有用的数据放入n[i]。空格是分隔符不需要放入trash，会被自动忽略，所以只需要两次放入trash
            Vec3f n;
            for (int i=0;i<3;i++) iss >> n[i];  // 每个法线Vn 由（x,y,z）三个元素组成。
            norms_.push_back(n);
        } else if (!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;              //把'v','t'先放入trash，然后再把有用的数据放入uv[i]。
            Vec2f uv;
            for (int i=0;i<2;i++) iss >> uv[i]; // 每个纹理坐标Vt 由（u,v）两个元素组成。
            uv_.push_back(uv);
        }  else if (!line.compare(0, 2, "f ")) {
            std::vector<Vec3i> f;
            Vec3i tmp;
            iss >> trash;   //先把'f'放入trash
            while (iss >> tmp[0] >> trash >> tmp[1] >> trash >> tmp[2]) {//对于f:283/266/283，把数字放入tmp，把'/'放入trash
                for (int i=0; i<3; i++) tmp[i]--; // in wavefront obj all indices start at 1, not zero
                f.push_back(tmp);
            }
            faces_.push_back(f);
        }
    }
    std::cerr << "# v# " << verts_.size() << " f# "  << faces_.size() << " vt# " << uv_.size() << " vn# " << norms_.size() << std::endl;
    load_texture(filename, "_diffuse.tga", diffusemap_);
}

Model::~Model() {
}

int Model::nverts() {
    return (int)verts_.size();
}

int Model::nfaces() {
    return (int)faces_.size();
}

std::vector<int> Model::face(int idx) {
    std::vector<int> face;
    for (int i=0; i<(int)faces_[idx].size(); i++) face.push_back(faces_[idx][i][0]);
    return face;
}

Vec3f Model::vert(int i) {
    return verts_[i];
}

void Model::load_texture(std::string filename, const char *suffix, TGAImage &img) {
    std::string texfile(filename);
    size_t dot = texfile.find_last_of(".");
    if (dot!=std::string::npos) {
        texfile = texfile.substr(0,dot) + std::string(suffix);
        std::cerr << "texture file " << texfile << " loading " << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
        img.flip_vertically();
    }
}

TGAColor Model::diffuse(Vec2i uv) {
    return diffusemap_.get(uv.x, uv.y);
}

Vec2i Model::uv(int iface, int nvert) {
    int idx = faces_[iface][nvert][1];
    return Vec2i(uv_[idx].x*diffusemap_.get_width(), uv_[idx].y*diffusemap_.get_height());
}

