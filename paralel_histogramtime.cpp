#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <iomanip>
#include <thread>
#include <string>
#include <chrono>
#include <fstream>

using namespace cv;
using namespace std;

void histogramHesapla(Mat parca, unsigned long long* histogram,
                     DWORD_PTR maske, bool* basarili)
{
    if (SetThreadAffinityMask(GetCurrentThread(), maske) == 0)
        return;

    for (int satir = 0; satir < parca.rows; satir++)
    {
        for (int sutun = 0; sutun < parca.cols; sutun++)
        {
            int parlaklik = parca.at<uchar>(satir, sutun);
            histogram[parlaklik]++;
        }
    }
    *basarili = true;
}

Mat histogramCiz(const unsigned long long* histogram, string baslik,
                unsigned long long enBuyuk)
{
    Mat grafik(330, 600, CV_8UC3, Scalar(255, 255, 255));
    putText(grafik, baslik, Point(15, 25), FONT_HERSHEY_SIMPLEX,
            0.6, Scalar(0, 0, 0), 1);
    putText(grafik, "Ust sinir (piksel): " + to_string(enBuyuk),
            Point(15, 50), FONT_HERSHEY_SIMPLEX, 0.45, Scalar(0, 0, 0), 1);

    for (int deger = 0; deger < 256; deger++)
    {
        if (histogram[deger] == 0)
            continue;
        int yukseklik = cvRound(200.0 * histogram[deger] / enBuyuk);
        if (yukseklik < 1)
            yukseklik = 1;
        int x = 45 + deger * 2;
        rectangle(grafik, Point(x, 279 - yukseklik), Point(x + 1, 278),
                  Scalar(180, 90, 30), FILLED);
    }
    line(grafik, Point(44, 79), Point(44, 279), Scalar(0, 0, 0));
    line(grafik, Point(44, 279), Point(557, 279), Scalar(0, 0, 0));
    int etiketler[] = {0, 64, 128, 192, 255};
    for (int deger : etiketler)
        putText(grafik, to_string(deger), Point(40 + deger * 2, 300),
                FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 0, 0), 1);
    putText(grafik, "Parlaklik degeri", Point(215, 323),
            FONT_HERSHEY_SIMPLEX, 0.45, Scalar(0, 0, 0), 1);
    return grafik;
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
    DWORD_PTR maskeler[4] = {};
    int sayi = 0;
    for (int i = 0; i < static_cast<int>(sizeof(DWORD_PTR) * 8) && sayi < 4; i++)
    {
        DWORD_PTR maske = DWORD_PTR(1) << i;
        if (islemMaskesi & maske)
            maskeler[sayi++] = maske;
    }
    if (sayi < 4)
    {
        cout << "Dort kullanilabilir mantiksal islemci gerekiyor." << endl;
        return 1;
    }

    int ortaX = resim.cols / 2;
    int ortaY = resim.rows / 2;
    Rect bolgeler[4] = {
        Rect(0, 0, ortaX, ortaY),
        Rect(ortaX, 0, resim.cols - ortaX, ortaY),
        Rect(0, ortaY, ortaX, resim.rows - ortaY),
        Rect(ortaX, ortaY, resim.cols - ortaX, resim.rows - ortaY)
    };

    // Her parcanin 0-255 arasindaki degerler icin ayri sayaclari var.
    unsigned long long histogramlar[4][256] = {};
    unsigned long long toplamHistogram[256] = {};
    unsigned long long anaHistogram[256] = {};
    thread isler[4];
    bool basarili[4] = {};

    auto paralelBaslangic = chrono::steady_clock::now();
    for (int i = 0; i < 4; i++)
        isler[i] = thread(histogramHesapla, resim(bolgeler[i]),
                         histogramlar[i], maskeler[i], &basarili[i]);

    for (int i = 0; i < 4; i++)
        isler[i].join();

    for (int i = 0; i < 4; i++)
    {
        if (!basarili[i])
        {
            cout << "Parca " << i + 1 << " islemciye atanamadi." << endl;
            return 1;
        }
    }

    // Ayni parlaklik degerinin dort parcadaki sayilarini topla.
    for (int deger = 0; deger < 256; deger++)
        for (int parca = 0; parca < 4; parca++)
            toplamHistogram[deger] += histogramlar[parca][deger];
    auto paralelBitis = chrono::steady_clock::now();

    // Kontrol icin ana resmi bolmeden, bastan say.
    auto seriBaslangic = chrono::steady_clock::now();
    for (int satir = 0; satir < resim.rows; satir++)
        for (int sutun = 0; sutun < resim.cols; sutun++)
            anaHistogram[resim.at<uchar>(satir, sutun)]++;
    auto seriBitis = chrono::steady_clock::now();

    double paralelMs = chrono::duration<double, milli>(paralelBitis - paralelBaslangic).count();
    double seriMs = chrono::duration<double, milli>(seriBitis - seriBaslangic).count();

    unsigned long long toplamPiksel = 0;
    unsigned long long parcaToplamlari[4] = {};
    unsigned long long enBuyuk = 1;
    bool ayni = true;
    cout << setw(8) << "Deger" << setw(12) << "Sol ust"
         << setw(12) << "Sag ust" << setw(12) << "Sol alt"
         << setw(12) << "Sag alt" << setw(12) << "Toplam"
         << setw(12) << "Ana resim" << endl;

    for (int deger = 0; deger < 256; deger++)
    {
        cout << setw(8) << deger;
        for (int i = 0; i < 4; i++)
        {
            cout << setw(12) << histogramlar[i][deger];
            parcaToplamlari[i] += histogramlar[i][deger];
        }
        cout << setw(12) << toplamHistogram[deger]
             << setw(12) << anaHistogram[deger] << '\n';
        toplamPiksel += toplamHistogram[deger];
        if (toplamHistogram[deger] != anaHistogram[deger])
            ayni = false;
        if (toplamHistogram[deger] > enBuyuk)
            enBuyuk = toplamHistogram[deger];
    }

    bool parcalarDogru = true;
    for (int i = 0; i < 4; i++)
    {
        unsigned long long beklenen = resim(bolgeler[i]).total();
        cout << "Parca " << i + 1 << ": " << parcaToplamlari[i]
             << " / beklenen: " << beklenen << endl;
        if (parcaToplamlari[i] != beklenen)
            parcalarDogru = false;
    }
    cout << "Histogramdaki toplam piksel: " << toplamPiksel << endl;
    cout << "Resimdeki toplam piksel: " << resim.total() << endl;
    cout << "256 degerin tamami ayni mi? " << (ayni ? "EVET" : "HAYIR") << endl;
    bool dogru = ayni && parcalarDogru && toplamPiksel == resim.total();
    cout << "Kontrol: " << (dogru ? "BASARILI" : "HATA") << endl;

    cout << fixed << setprecision(6);
    cout << "\n--- SURE OLCUMU (tek calistirma) ---" << endl;
    cout << "Seri histogram: " << seriMs << " ms" << endl;
    cout << "4 is parcacigi + birlestirme: " << paralelMs << " ms" << endl;
    if (paralelMs > 0)
        cout << "Hizlanma orani (seri / paralel): " << seriMs / paralelMs << endl;
    cout << "Oran 1'den buyukse paralel, kucukse seri daha hizlidir." << endl;

    ofstream rapor("histogram_olcum.txt");
    if (rapor)
    {
        rapor << fixed << setprecision(6);
        rapor << "Resim: resim.jpg\nBoyut: " << resim.cols << " x " << resim.rows << '\n';
        rapor << "Resimdeki toplam piksel: " << resim.total() << '\n';
        rapor << "Histogramdaki toplam piksel: " << toplamPiksel << '\n';
        rapor << "256 degerin tamami ayni mi? " << (ayni ? "EVET" : "HAYIR") << '\n';
        rapor << "Kontrol: " << (dogru ? "BASARILI" : "HATA") << '\n';
        rapor << "Seri histogram: " << seriMs << " ms\n";
        rapor << "4 is parcacigi + birlestirme: " << paralelMs << " ms\n";
        if (paralelMs > 0)
            rapor << "Hizlanma orani (seri / paralel): " << seriMs / paralelMs << '\n';
        rapor << "Tek calistirmaya ait olcum; kesin performans sonucu degildir.\n";
        rapor << "Paralel sureye thread olusturma, sabitleme, bekleme ve birlestirme dahildir.\n";
        rapor << "Resim okuma, konsola yazma ve grafik cizme dahil degildir.\n";
        rapor.close();
        if (!rapor)
            cout << "Olcum dosyasi tam yazilamadi." << endl;
    }
    else
        cout << "Olcum dosyasi acilamadi." << endl;

    // Grafiklerin hepsinde ayni dikey olcek kullanilir.
    Mat grafikler[6];
    string adlar[6] = {"1 - Sol ust", "2 - Sag ust", "3 - Sol alt",
                      "4 - Sag alt", "Birlestirilen histogram", "Ana resmin histogrami"};
    for (int i = 0; i < 4; i++)
        grafikler[i] = histogramCiz(histogramlar[i], adlar[i], enBuyuk);
    grafikler[4] = histogramCiz(toplamHistogram, adlar[4], enBuyuk);
    grafikler[5] = histogramCiz(anaHistogram, adlar[5], enBuyuk);

    Mat satirlar[3], sonuc;
    for (int i = 0; i < 3; i++)
        hconcat(grafikler[i * 2], grafikler[i * 2 + 1], satirlar[i]);
    vconcat(satirlar, 3, sonuc);
    namedWindow("Histogramlar", WINDOW_NORMAL);
    imshow("Histogramlar", sonuc);
    resizeWindow("Histogramlar", 960, 792);
    if (!imwrite("histogram_sonuc.png", sonuc))
        cout << "Sonuc resmi kaydedilemedi." << endl;
    waitKey(0);
    destroyAllWindows();
    return dogru ? 0 : 1;
}
