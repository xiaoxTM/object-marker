#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <libu/u-options>
#include <libu/u-log>
#include <libu/u-path>
#include <libu/u-io>

cv::Rect box;
cv::Mat preview_image;
cv::Mat current_image;
cv::Mat original_image;

std::vector<cv::Rect> boxes;

int ivec = -1;

cv::Scalar color;
cv::Scalar font_color;

double font_scale = 1.0;
int line_type = 8;
int font_thickness = 2;

bool toggle = true;

static void on_mouse_click(int event, int x, int y, int flags, void *userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        box.x = x;
        box.y = y;
    } else if (event == cv::EVENT_LBUTTONUP) {
        cv::Size *size = reinterpret_cast<cv::Size *>(userdata);
        if (size->width > 0 && size->height > 0) {
            box.x = x;
            box.y = y;
            
            box.width = size->width;
            box.height = size->height;
            if (box.x + box.width >= original_image.cols) box.width = original_image.cols - box.x;
            if (box.y + box.height >= original_image.rows) box.height = original_image.rows - box.y;
            
            if (! current_image.empty()) {
                current_image.release();
            }
        
            current_image = original_image.clone();

            cv::Point begin(box.x, box.y);
            cv::Point end(box.x + box.width, box.y + box.height);

            cv::rectangle(current_image, begin, end, color);
            cv::imshow("image", current_image);
        } else {
            box.width = std::abs(box.x - x);
            box.height = std::abs(box.y - y);
            box.x = std::min(box.x, x);
            box.y = std::min(box.y, y);
        }
        
    } else if (flags == cv::EVENT_FLAG_LBUTTON && event == cv::EVENT_MOUSEMOVE) {
        cv::Size *size = reinterpret_cast<cv::Size *>(userdata);
        if (size->width <= 0 && size->height <= 0) {
            cv::Point begin(std::min(box.x, x), std::min(box.y, y));
            cv::Point end(std::max(box.x, x), std::max(box.y, y));
            if (! current_image.empty()) {
                current_image.release();
            }
        
            current_image = original_image.clone();
            cv::rectangle(current_image, begin, end, color);
            cv::imshow("image", current_image);
        }
    }
}

void update_preview(bool put_text) {
    if (! preview_image.empty())
        preview_image.release();
    preview_image = original_image.clone();
    for (size_t i=0; i<boxes.size(); ++i) {
        if (put_text) {
            std::string text = std::to_string(i);
            int baseline = 0;
            cv::Size size = cv::getTextSize(text, cv::FONT_HERSHEY_SCRIPT_SIMPLEX, font_scale, font_thickness, NULL);
            cv::Point coord(boxes[i].x + (std::abs(boxes[i].width - size.width) >> 1), boxes[i].y + (std::abs(boxes[i].height + size.height) >> 1));
            cv::putText(preview_image, text, coord, cv::FONT_HERSHEY_SCRIPT_SIMPLEX, font_scale, font_color, font_thickness, line_type);
        }
        cv::rectangle(preview_image, cv::Point(boxes[i].x, boxes[i].y), cv::Point(boxes[i].x + boxes[i].width, boxes[i].y + boxes[i].height), color);
    }
    cv::imshow("preview", preview_image);
}

bool list_boundingbox_stream(const std::string &filename, std::ofstream &ofs, int &mode, bool &preview, bool &put_text) {
    boxes.clear();
    if (! original_image.empty()) {
        original_image.release();
    }
    
    original_image = cv::imread(filename);
    assert(! original_image.empty());
    if (! current_image.empty()) {
        current_image.release();
    }
    
    current_image = original_image.clone();
    cv::imshow("image", current_image);
    if (preview) {
        if (! preview_image.empty()) {
            preview_image.release();
        }
        preview_image = original_image.clone();
        cv::imshow("preview", preview_image);
    }
    
    int key = 0;
    while (true) {
        key = cv::waitKey(0);
        u::log::echo<0x8000>("key: %i pressed\n", key);
        
        if (key == u::key("q")) { // quit program
            ofs.close(); // hope will never come here
            exit(0);
        } else if (key == u::key("ENTER")) { // save mark and go next image
            box.width = 0;
            box.height = 0;
            if (! current_image.empty()) {
                current_image.release();
            }
        
            current_image = original_image.clone();
            cv::imshow("image", current_image);
            ivec = -1;
            break;
        } else if (key == u::key("ESC")) { //skip current box
            box.width = 0;
            box.height = 0;
            if (!current_image.empty()) {
                current_image.release();
            }
            current_image = original_image.clone();
            cv::imshow("image", current_image);
            ivec = -1;
        } else if (key == u::key("SPACE")) { // mark current box
            if (box.width > 0 && box.height > 0) {
                boxes.push_back(box);
            }
            if (preview) {
                if (put_text) {  
                    std::string text = std::to_string(boxes.size() - 1);
                    cv::Size size = cv::getTextSize(text, cv::FONT_HERSHEY_SCRIPT_SIMPLEX, font_scale, font_thickness, NULL);
                    cv::Point coord(box.x + (std::abs(box.width - size.width) >> 1), box.y + (std::abs(box.height + size.height) >> 1));
                    cv::putText(preview_image, text, coord, cv::FONT_HERSHEY_SCRIPT_SIMPLEX, font_scale, font_color, font_thickness, line_type);
                }
                cv::rectangle(preview_image, cv::Point(box.x, box.y), cv::Point(box.x + box.width, box.y + box.height), color);
                cv::imshow("preview", preview_image);
            }
            box.width = 0;
            box.height = 0;
            ivec = -1;
        } else if (key == u::key("d")) {
            if (ivec >= 0 && ivec < boxes.size()) {
                boxes.erase(boxes.begin() + ivec);
            }
            if (preview) {
                update_preview(put_text);
            }
            ivec = -1;
        }else if (key >= u::key("0") && key <= u::key("9")) {
            if(ivec < 0) {
                ivec = key - u::key("0");
            } else {
                ivec = ivec * 10 + key - u::key("0");
            }
        } else if (key == u::key("p")) {
            if (preview) {
                if (toggle) {
                    cv::destroyWindow("preview");
                } else {
                    cv::namedWindow("preview", cv::WINDOW_NORMAL);
                    update_preview(put_text);
                }
                toggle = !toggle;
            }
            ivec = -1;
        }
    }

    // save marked boxes
    if (mode == 0) {
        ofs << filename << " " << boxes.size() << " ";
        for (size_t i=0; i<boxes.size(); ++i) {
            ofs << boxes[i].x << " " << boxes[i].y << " " << boxes[i].width << " " << boxes[i].height;
            if (i+1 != boxes.size()) {
                ofs << " ";
            }
        }
        ofs << std::endl;
    } else {
        for (size_t i=0; i<boxes.size(); ++i) {
            ofs << boxes[i].x << " " << boxes[i].y << " " << boxes[i].width << " " << boxes[i].height;
            if (i+1 != boxes.size()) {
                ofs << std::endl;
            }
        }
    }
    
    return true;
}

bool list_boundingbox_file(const std::string &filename, const std::string &fold, int &mode, bool &preview, bool &put_text) {
    assert(u::string::end_with(true, filename, {"jpg", "jpeg", "bmp", "png"}));
    
    u::path path(filename);
    
    std::string file = u::path::join({fold, std::string(path.get(u::F)) + ".om"});
    std::ofstream ofs(file);
    u_assert(!ofs.fail(), u::format("Open file failed. %s. Make sure you give the absolute path.", file.c_str()));
    list_boundingbox_stream(filename, ofs, mode, preview, put_text);
    ofs.close();
    return true;
}

bool walk_boundingbox_stream(const std::string &root, const std::string &parent, const std::string &filename, unsigned char flag, std::ofstream &ofs, int &mode, bool &preview, bool &put_text) {
    bool ret = true;
    if ((flag & u::F) == u::F) {
        std::string file = u::path_cat(root, parent, filename);
        ret = list_boundingbox_stream(file, ofs, mode, preview, put_text);
    }
    return ret;
}

bool walk_boundingbox_file(const std::string &root, const std::string &parent, const std::string &filename, unsigned char flag, const std::string &fold, int &mode, bool &preview, bool &put_text) {
    bool ret = true;
    if ((flag & u::F) == u::F) {
        std::string file = u::path_cat(root, parent, filename);
        ret = list_boundingbox_file(file, fold, mode, preview, put_text);
    }
    return ret;
}

void init_window(cv::Size &size, bool preview) {
    cv::namedWindow("image", cv::WINDOW_NORMAL);
    cv::setMouseCallback("image", on_mouse_click, reinterpret_cast<void*>(&size));
    
    if (preview) {
        cv::namedWindow("preview", cv::WINDOW_NORMAL);
    }
}

void print_help() {
    u::log::term(0, 0, "object marker for marking object with bounding box.\n");
    u::log::term(0, 0, "usage:\n");
    u::log::term(0, 0, "    q: exit this program\n");
    u::log::term(0, 0, "    ESC: unsubmit current bounding box\n");
    u::log::term(0, 0, "    SPACE: mark drawn bounding box\n");
    u::log::term(0, 0, "    ENTER: submit bounding boxes and load next image\n");
    u::log::term(0, 0, "    [i]d: delete i-th box, where i=[0-9]+\n");
    u::log::term(0, 0, "    p: display / destroy preview windows. NOTE: this only works when preview is true\n");
}

int main(int argc, char *argv[]) {
    char *source = NULL;
    char *destinate = NULL;
    bool preview = true;
    bool put_text = true;
    bool debug = false;
    
    int width = -100;
    int height = -100;
    int mode = 100;
    
    std::vector<u::options::entry*> options;
    options.push_back(u::options::entry::create<char*>(&source, "--source", "-s", NULL, "source specifying images to be marked"));
    options.push_back(u::options::entry::create<char*>(&destinate, "--destinate", "-o", NULL, "destinate to store marking information"));
    options.push_back(u::options::entry::create<bool>(&preview, "--preview", "-p", false, "switch to open/close preview of marked object"));
    options.push_back(u::options::entry::create<bool>(&put_text, "--draw-text", "-t", false, "switch to draw text to preview of marked object"));
    options.push_back(u::options::entry::create<bool>(&debug, "--debug-mode", "-d", false, "debug mode"));

    options.push_back(u::options::entry::create<double>(&font_scale, "--font-scale", "-f", 1.0, "font scale when draw order to box"));
    
    options.push_back(u::options::entry::create<int>(&font_thickness, "--font-thickness", NULL, 2, "stroke for draw text for bounding box"));
    options.push_back(u::options::entry::create<int>(&line_type, "--line-type", "-l", 8, "line type for draw text for bounding box"));
    options.push_back(u::options::entry::create<int>(&width, "--fixed-width", "-w", -1, "width of bounding box, only effect iif with and height positive"));
    options.push_back(u::options::entry::create<int>(&height, "--fixed-height", "-h", -1, "height of bounding box, only used iiif width and height are positive"));
    options.push_back(u::options::entry::create<int>(&mode, "--mode", "-m", 0, "mode of storing mark information. 0: all in one file. In this case, destinate must be file\n \
                                                                                1: one mark file for one image. In this case, destinate must be directory"));

    if (argc == 1) {
        u::options::help("object-marker", options);
    } else {
        u::options::parse(argc, argv, options);
        if (argc != 1) {
            u::log::term(0, 0, "Warning: unknown options:");
            for (int i=1; i<argc; ++i) {
                u::log::term(0, 0, "%i -->  %s", i, argv[i]);
            }
        }
        u_assert(source != NULL && destinate != NULL, "FATAL: source / destinate not given");
        cv::Size size(width, height);

        print_help();
        u::log::term(0, 0, "press any key to continue:");
        std::cin.get();

        if (debug) {
            unsigned short flag = u::T;
            flag |= (0x8000 | u::FLUSH);
            u::log::open(flag);
        }
        
        init_window(size, preview);
        color = cv::Scalar(255, 0, 0);
        font_color = cv::Scalar(0, 0, 255);
        if (u::path::exists(source, u::F)) {
            if (u::string::end_with(true, source, {"jpg", "jpeg", "bmp", "png"})) { // given single image file
                list_boundingbox_file(source, destinate, mode, preview, put_text);
            } else if (u::string::end_with(true, destinate, {"txt", "list", "db"})) { // given text file including image list
                std::vector<std::string> storage;
                u::io::loadrec(source, storage);
                if (mode == 0) {
                    std::ofstream ofs(destinate);
                    u_assert(!ofs.fail(), u::format("Open file failed. %s. Make sure you give the absolute path.", destinate));
                    u::list(storage, list_boundingbox_stream, ofs, mode, preview, put_text);
                    ofs.close();
                } else {
                    u_assert(u::path::exists(destinate, u::D), u::format("FATAL: directory required, but given file: %s", destinate));
                    u::list(storage, list_boundingbox_file, destinate, mode, preview, put_text);
                }
            } else {
                bool UNSUPPORT_FILE_FORMAT = false;
                u_assert(UNSUPPORT_FILE_FORMAT, u::format("FATAL: unsupport file with suffix: %s", source));
            }
        } else if (u::path::exists(source, u::D)) {
            if (mode == 0) {
                std::ofstream ofs(destinate);
                u_assert(!ofs.fail(), u::format("Open file failed: %s. Make sure you give the absolute path.", destinate));
                u::walk(0, source, walk_boundingbox_stream, ofs, mode, preview, put_text);
                ofs.close();
            } else {
                u_assert(u::path::exists(destinate, u::D), u::format("FATAL: directory required, but given file: %s", destinate));
                u::walk(0, source, walk_boundingbox_file, destinate, mode, preview, put_text);
            }
        } else {
            bool NO_SUCH_FILE_OR_DIRECTORY = false;
            u_assert(NO_SUCH_FILE_OR_DIRECTORY, u::format("FATAL: no such file or directory: %s", source));
        }
        if (debug) {
            u::log::close();
        }
    }    
}
