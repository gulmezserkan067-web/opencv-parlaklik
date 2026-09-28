#include <opencv2/opencv.hpp>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    std::string klasor = "resimler";
    bool pencereAc = true;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--penceresiz") pencereAc = false;
        else klasor = argv[i];
    }

    try {
        if (!fs::is_directory(klasor)) {
            std::cout << "Resim klasoru bulunamadi.\n";
            return 1;
        }
        fs::create_directories("sonuclar");
        if (fs::equivalent(klasor, "sonuclar")) {
            std::cout << "Giris olarak sonuc klasorunu secmeyin.\n";
            return 1;
        }

        int sayac = 0;
        for (const auto& dosya : fs::directory_iterator(klasor)) {
            if (!dosya.is_regular_file()) continue;
            cv::Mat resim = cv::imread(dosya.path().string());
            if (resim.empty()) continue;  // resim olmayan dosyalari atla

            cv::Mat yeniResim;
            cv::resize(resim, yeniResim, cv::Size(1024, 768));

            // Farkli uzantili ama ayni isimli dosyalar birbirinin ustune yazilmasin.
            std::string ad = dosya.path().filename().string();
            std::string kayitYolu = "sonuclar/" + ad + ".png";
            if (!cv::imwrite(kayitYolu, yeniResim)) return 1;
            std::cout << ad << " : " << resim.cols << "x" << resim.rows
                      << " -> " << yeniResim.cols << "x" << yeniResim.rows << '\n';
            sayac++;

            if (pencereAc) {
                cv::imshow("Boyutlandirilmis resim", yeniResim);
                cv::waitKey(0);  // bir tusa basinca diger resme gec
            }
        }
        if (pencereAc) cv::destroyAllWindows();
        std::cout << sayac << " resim islendi.\n";
        return sayac == 0 ? 1 : 0;
    } catch (const std::exception& hata) {
        std::cerr << "Hata: " << hata.what() << '\n';
        return 1;
    }
}
