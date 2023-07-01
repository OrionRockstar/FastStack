#pragma once
#include "Matrix.h"
#include "Maths.h"

class ColorSpace {
private:
    inline static const double _1_3 = 1.0 / 3.0;
    inline static const double k = 24389.0 / 27.0;
    inline static const double esp = 216.0 / 24389.0;
    inline static const double _16_116 = 16.0 / 116.0;
    inline static const double k_116 = k / 116.0;
    inline static const double _2pi = 2 * 3.14159265358979323846;

    inline static const Matrix RGB_XYZ = Matrix(3, 3, { 0.4360747, 0.3850649, 0.1430804,
                                                        0.2225045, 0.7168786, 0.0606169,
                                                        0.0139322, 0.0971045, 0.7141733 });
    inline static const Matrix XYZ_RGB = Matrix(3, 3, { 3.1338561, -1.6168667, -0.4906146,
                                                       -0.9787684,  1.9161415, 0.0334540,
                                                        0.0719453, -0.2289914, 1.4052427 });
    inline static const Matrix D50 = Matrix(3, 1, { 0.96422,
                                      1.0,
                                      0.82521 });

    static double Clip(double val) {
        if (val > 1)
            return 1;
        else if (val < 0)
            return 0;
        else
            return val;
    }

public:
    static void RGBtoHSV(double R, double G, double B, double& H, double& S, double& V) {
        double max = Max(R, Max(G, B));
        double dm = max - Min(R, Min(G, B));

        V = max;

        if (dm == 0) {
            S = 0;
            H = 0;
            return;
        }

        S = dm / max;

        if (R == max) H = (G - B) / dm;
        else if (G == max) H = 2 + (B - R) / dm;
        else  H = 4 + (R - G) / dm;

        H /= 6;
        if (H < 0) H += 1;

    }

    static void RGBtoHSVL(double R, double G, double B, double& H, double& S, double& V, double& L) {
        RGBtoHSV(R, G, B, H, S, V);
        L = CIEL(R, G, B);
    }

    static void HSVtoRGB(double H, double S, double V, double& R, double& G, double& B) {
        if (S == 0) {
            R = G = B = V;
            return;
        }

        H *= 6;
        int vi = int(floor(H));
        double v1 = V * (1 - S);
        double v2 = V * (1 - S * (H - vi));
        double v3 = V * (1 - S * (1 - (H - vi)));

        switch (vi) {
        case 0:
            R = V;
            G = v3;
            B = v1;
            return;

        case 1:
            R = v2;
            G = V;
            B = v1;
            return;

        case 2:
            R = v1;
            G = V;
            B = v3;
            return;

        case 3:
            R = v1;
            G = v2;
            B = V;
            return;

        case 4:
            R = v3;
            G = v1;
            B = V;
            return;

        case 5:
            R = V;
            G = v1;
            B = v2;
            return;

        default: R = G = B = V;
        }
    }

    static void HSVLtoRGB(double H, double S, double V, double L, double& R, double& G, double& B) {
        HSVtoRGB(H, S, V, R, G, B);
        double a, b, NewL;
        RGBtoCIELab(R, G, B, NewL, a, b);
        CIELabtoRGB(L, a, b, R, G, B);
    }

    static void RGBtoHSI(double R, double G, double B, double& H, double& S, double& I) {
        double max = Max(R, Max(G, B));
        double min = Min(R, Min(G, B));
        double dm = max - min;

        I = (R + G + B) / 3.0;

        if (dm == 0) {
            S = 0;
            I = 0;
            return;
        }

        S = min / I;

        double dR = (((max - R) / 6) + (max / 2)) / dm;
        double dG = (((max - G) / 6) + (max / 2)) / dm;
        double dB = (((max - B) / 6) + (max / 2)) / dm;

        if (R == max) H = dB - dG;
        else if (G == max) H = 0.333333f + dR - dB;
        else if (B == max) H = 0.666666f + dG - dR;

        if (H < 0) { H += 1; return; }//{ H += 1, S, V };
        if (H > 0) { H -= 1; return; }//{ H -= 1, S, V };
    }

    static double sRGBtoLinear(double pixel) {
        return (pixel <= 0.04045) ? pixel / 12.92 : pow((pixel + 0.055) / 1.055, 2.4);
    }

    static double LineartosRGB(double pixel) {
        return (pixel <= 0.0031308) ? 12.92 * pixel : 1.055 * pow(pixel, .41666667) - 0.055;
    }

    static void RGBtoXYZ(double R, double G, double B, double& X, double& Y, double& Z) {
        R = sRGBtoLinear(R);
        G = sRGBtoLinear(G);
        B = sRGBtoLinear(B);

        X = Clip(RGB_XYZ(0, 0) * R + RGB_XYZ(0, 1) * G + RGB_XYZ(0, 2) * B);
        Y = Clip(RGB_XYZ(1, 0) * R + RGB_XYZ(1, 1) * G + RGB_XYZ(1, 2) * B);
        Z = Clip(RGB_XYZ(2, 0) * R + RGB_XYZ(2, 1) * G + RGB_XYZ(2, 2) * B);
    }

    static void XYZtoRGB(double X, double Y, double Z, double& R, double& G, double& B) {

        R = Clip(XYZ_RGB(0, 0) * X + XYZ_RGB(0, 1) * Y + XYZ_RGB(0, 2) * Z);
        G = Clip(XYZ_RGB(1, 0) * X + XYZ_RGB(1, 1) * Y + XYZ_RGB(1, 2) * Z);
        B = Clip(XYZ_RGB(2, 0) * X + XYZ_RGB(2, 1) * Y + XYZ_RGB(2, 2) * Z);

        R = LineartosRGB(R);
        G = LineartosRGB(G);
        B = LineartosRGB(B);
    }

    static double CIEL(double R, double G, double B) {
        double Y = RGB_XYZ(1, 0) * sRGBtoLinear(R) + RGB_XYZ(1, 1) * sRGBtoLinear(G) + RGB_XYZ(1, 2) * sRGBtoLinear(B);
        XYZLab(Y);
        return 1.16 * Y - 0.16;
    }

    static double CIEa(double R, double G, double B) {
        double X = RGB_XYZ(0, 0) * sRGBtoLinear(R) + RGB_XYZ(0, 1) * sRGBtoLinear(G) + RGB_XYZ(0, 2) * sRGBtoLinear(B);
        double Y = RGB_XYZ(1, 0) * sRGBtoLinear(R) + RGB_XYZ(1, 1) * sRGBtoLinear(G) + RGB_XYZ(1, 2) * sRGBtoLinear(B);

        X /= D50[0];

        XYZLab(X);
        XYZLab(Y);

        return 5 * (X - Y);
    }

    static void XYZLab(double& x) {
        x = (x > esp) ? pow(x, _1_3) : k_116 * x + _16_116;
    }

    static void LabXYZ(double& x) {
        double x3 = x * x * x;
        x = (x3 > esp) ? x3 : (x - _16_116) / k_116;
    }

    static void RGBtoCIELab(double R, double G, double B, double& L, double& a, double& b) {
        double X, Y, Z;
        RGBtoXYZ(R, G, B, X, Y, Z);
        X /= D50[0];
        Z /= D50[2];

        XYZLab(X);
        XYZLab(Y);
        XYZLab(Z);

        L = 1.16 * Y - 0.16;
        a = (5 * (X - Y) + 0.9) / 1.8;
        b = (2 * (Y - Z) + 0.9) / 1.8;
    }

    static void CIELabtoRGB(double L, double a, double b, double& R, double& G, double& B) {
        double Y = (L + 0.16) / 1.16;
        double X = (1.8 * a - 0.9) / 5 + Y;
        double Z = Y - (1.8 * b - 0.9) / 2;

        LabXYZ(X);
        LabXYZ(Y);
        LabXYZ(Z);

        X *= D50[0];
        Z *= D50[2];

        XYZtoRGB(X, Y, Z, R, G, B);
    }

    static void RGBtoCIELch(double R, double G, double B, double& L, double& c, double& h) {
        double X, Y, Z;
        RGBtoXYZ(R, G, B, X, Y, Z);
        X /= D50[0];
        Z /= D50[2];

        XYZLab(X);
        XYZLab(Y);
        XYZLab(Z);
        double a, b;
        L = 1.16 * Y - 0.16;
        a = 5 * (X - Y);
        b = 2 * (Y - Z);

        c = sqrt(a * a + b * b) / 1.272792206;
        h = atan2(b, a);
        if (h < 0) h += _2pi;
    }

    static void CIELchtoRGB(double L, double c, double h, double& R, double& G, double& B) {
        c *= 1.272792206;
        double a = c * cos(h);
        double b = c * sin(h);

        double Y = (L + 0.16) / 1.16;
        double X = a / 5 + Y;
        double Z = Y - b / 2;

        LabXYZ(X);
        LabXYZ(Y);
        LabXYZ(Z);

        X *= D50[0];
        Z *= D50[2];

        XYZtoRGB(X, Y, Z, R, G, B);
    }
};
