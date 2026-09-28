#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

int main()
{
    Mat resim = imread("resim.jpg", IMREAD_GRAYSCALE);

    if (resim.empty())
    {
        cout << "Resim acilamadi" << endl;
        return 1;
    }

    Mat bolunmus = resim.clone();
    Mat carpilmis = resim.clone();

    // Her pikselin parlaklik degerini oku
    for (int satir = 0; satir < resim.rows; satir++)
    {
        for (int sutun = 0; sutun < resim.cols; sutun++)
        {
            int parlaklik = resim.at<uchar>(satir, sutun);

            bolunmus.at<uchar>(satir, sutun) = parlaklik / 4;

            int yeniDeger = parlaklik * 4;
            if (yeniDeger > 255)
            {
                yeniDeger = 255;
            }
            carpilmis.at<uchar>(satir, sutun) = yeniDeger;
        }
    }

    imshow("Orijinal", resim);
    imshow("4'e bolunmus", bolunmus);
    imshow("4 ile carpilmis", carpilmis);

    waitKey(0);
    destroyAllWindows();
    return 0;
}
