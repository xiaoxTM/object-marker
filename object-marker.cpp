#include <opencv2/highgui/highgui.cpp>

#include <libu/u-options>

bool multifile = false;
bool preview = false;
char *color = NULL;
char *dest = NULL;


void mark_image(const cv::Mat &image, std::ofstream &ofs) 
{
}

void mark_image(const cv::Mat &image, const std::string &filename) 
{
    std::ofstream ofs(filename);
    u_assert(!ofs.fail(), u::format("open file %s failed", filename.c_str()));
    mark_image(image, ofs);
    ofs.close();
}

void mark_image_from_file(const std::string &filename) 
{
    std::vector<std::string> filenames;
    u::log::loadtxt(filename, filenames);
    if (multifile) {
        u_assert(u::path::exists(dest, u::D), u::format("Error: fold %s not exists.", dest));
        for (size_t i=0; i<filenames.size(); ++i) {
            u::path path(filename);
            std::string name(path.get("file"));
            std::string destname = u::path::join(dest, name+".txt");
            mark_image(filenames[i], destname);
        }
    } else {
        std::ofstream ofs(dest);
        u_assert(!ofs.empty(), u::format("open file: %s failed.", dest));
        for (size_t i=0; i<filenames.size(); ++i) {
            mark_image(filenames[i], dest);
        }
        ofs.close();
    }
}

bool mark(const std::string &root, const std::string &parent, const std::string &filename, unsigned char flag, std::ofstream &ofs) 
{
    if ((flag & u::F) == u::F && u::path::end_with(true, {"png", "bmp", "jpg", "jpeg", "tif", "tiff"}, filename)) {
        std::string name = u::path_cat(root, parent, filename);
        cv::Mat image = cv::imread(name);
        if (multifile) {
            u_assert(u::path::exists(dest, u::D), u::format("Error: fold %s not exists.", dest));
            for (size_t i=0; i<filenames.size(); ++i) {
                std::string name = u::path::replace(filename, "txt", "suffix");
                std::string destname = u::path::join(dest, name);
                mark_image(filenames[i], destname);
            }
        } else {
            mark_image(image, ofs);
        }
    }
    return 0;
}

int main(int argc, char *argv[]) 
{

    char *source = NULL;
    
    std::vector<u::options::entry*> options;
    options.push_back(u::options::entry::create<bool>(&preview, "bool", "--preview", "-p", "false", "preview markers been drawn"));
    options.push_back(u::options::entry::create<bool>(&multifile, "bool", "--multi-files", "-m", "false", "write markers to one file for each image, if true; otherwise, write all to one file"));
    options.push_back(u::options::entry::create<char*>(&color, "string", "--color", "-c", "red", "color to draw mark if preview"));
    options.push_back(u::options::entry::create<char*>(&source, "string", "--source", "-s", "$NULL", "source to mark. Can be single file or directory"));
    options.push_back(u::options::entry::create<char*>(&dest, "string", "--destinate", "-d", "$NULL", "destinate to store infomration. If source is file, destinate should also be file. If multi-files not set, destinate also will be a single file"));

    if (argc == 1) {
        u::options::help("object-marker", options);
    } else {
        u::options::parse(argc, argv, options);
        u_assert(source != NULL, "source must be given");
        u_assert(dest != NULL, "destinate must be given");
        if (u::path::exists(source, u::D)) { // if given directory
            u::walk(0, source, mark, dest);
        } else if (u::path::exists(source, u::F)) { // if given file
            if (u::path::end_with(true, {"png", "bmp", "jpg", "jpeg", "tif", "tiff"}, source)) { // if given single image
                std::ofstream ofs(dest);
                u_assert(!ofs.fail(), u::format("open file %s failed", dest));
                cv::Mat image = cv::imread(source);
                u_assert(!image.empty(), u::format("load image %s failed", source));
                mark_image(image, ofs);
                ofs.close();
            } else if (u::path::end_with(true, {"txt", "list", "db"}, source)) {
                mark_image_from_file(source);
            } else {
                u::log::fatal("FATAL: file format not support. You can either specify image file (png, bmp, jpg, tif) or text file (txt, list, db) who include image iamges");
            }
        }
    }

    return 0;
}
