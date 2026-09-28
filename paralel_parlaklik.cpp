#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <string>

using namespace cv;
using namespace std;

void parcayiIsle(Mat parca, Mat bolunmus, Mat carpilmis,
                DWORD_PTR maske, bool* basarili)
{
    // Bu is parcacigini tek bir mantiksal islemciye sabitle.
    if (SetThreadAffinityMask(GetCurrentThread(), maske) == 0)
        return;

    for (int satir = 0; satir < parca.rows; satir++)
    {
        for (int sutun = 0; sutun < parca.cols; sutun++)
        {
            int parlaklik = parca.at<uchar>(satir, sutun);
            bolunmus.at<uchar>(satir, sutun) = parlaklik / 4;

            int yeniDeger = parlaklik * 4;
            if (yeniDeger > 255)
                yeniDeger = 255;

            carpilmis.at<uchar>(satir, sutun) = yeniDeger;
        }
    }
    *basarili = true;
}

int main()
{
    Mat resim = imread("resim.jpg", IMREAD_GRAYSCALE);
    if (resim.empty() || resim.rows < 2 || resim.cols < 2)
    {
        cout << "Resim acilamadi veya boyutu cok kucuk." << endl;
        return 1;
    }

    DWORD_PTR islemMaskesi = 0, sistemMaskesi = 0;
    if (!GetProcessAffinityMask(GetCurrentProcess(), &islemMaskesi, &sistemMaskesi))
    {
        cout << "Islemci bilgisi okunamadi." << endl;
        return 1;
    }

    // Programin kullanabildigi ilk dort mantiksal islemciyi bul.
    DWORD_PTR maskeler[4] = {};
    int islemciler[4] = {};
    int sayi = 0;
    for (int i = 0; i < static_cast<int>(sizeof(DWORD_PTR) * 8) && sayi < 4; i++)
    {
        DWORD_PTR maske = DWORD_PTR(1) << i;
        if (islemMaskesi & maske)
        {
            maskeler[sayi] = maske;
            islemciler[sayi] = i;
            sayi++;
        }
    }
    if (sayi < 4)
    {
        cout << "En az dort kullanilabilir mantiksal islemci gerekiyor." << endl;
        return 1;
    }

    int ortaX = resim.cols / 2;
    int ortaY = resim.rows / 2;

    // Sol ust, sag ust, sol alt, sag alt.
    Rect bolgeler[4] = {
        Rect(0, 0, ortaX, ortaY),
        Rect(ortaX, 0, resim.cols - ortaX, ortaY),
        Rect(0, ortaY, ortaX, resim.rows - ortaY),
        Rect(ortaX, ortaY, resim.cols - ortaX, resim.rows - ortaY)
    };

    Mat parcalar[4], bolunmus[4], carpilmis[4];
    thread isler[4];
    bool basarili[4] = {};

    for (int i = 0; i < 4; i++)
    {
        parcalar[i] = resim(bolgeler[i]);
        bolunmus[i] = parcalar[i].clone();
        carpilmis[i] = parcalar[i].clone();

        isler[i] = thread(parcayiIsle, parcalar[i], bolunmus[i],
                         carpilmis[i], maskeler[i], &basarili[i]);
    }

    // Dort is de baslatildiktan sonra bitmelerini bekle.
    for (int i = 0; i < 4; i++)
        isler[i].join();

    for (int i = 0; i < 4; i++)
    {
        if (!basarili[i])
        {
            cout << "Parca " << i + 1 << " islemciye atanamadi." << endl;
            return 1;
        }
        cout << "Parca " << i + 1 << " -> mantiksal islemci "
             << islemciler[i] << endl;
    }

    // Pencereler ana is parcaciginda acilir.
    for (int i = 0; i < 4; i++)
    {
        string ad1 = "Parca " + to_string(i + 1) + " - 4'e bolunmus";
        string ad2 = "Parca " + to_string(i + 1) + " - 4 ile carpilmis";
        namedWindow(ad1, WINDOW_NORMAL);
        namedWindow(ad2, WINDOW_NORMAL);
        imshow(ad1, bolunmus[i]);
        imshow(ad2, carpilmis[i]);
        resizeWindow(ad1, 360, 260);
        resizeWindow(ad2, 360, 260);
        moveWindow(ad1, (i % 2) * 380, (i / 2) * 310);
        moveWindow(ad2, 780 + (i % 2) * 380, (i / 2) * 310);
    }

    waitKey(0);
    destroyAllWindows();
    return 0;
}
