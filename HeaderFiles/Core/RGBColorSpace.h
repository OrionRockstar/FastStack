#pragma once
#include "Matrix.h"
#include "Maths.h"

enum class ColorComponent : uint8_t {
    red = 0,
    green,
    blue,
    rgb_k,
    Lightness,
    a,
    b,
    c,
    hue,
    saturation
};


class ColorSpace {
public:
    ColorSpace() = delete;

    enum class Type : uint8_t {
        rgb,
        hsv,
        hsi,
        ciexyz,
        cielab,
        cielch
    };

    /*enum class Channel : uint8_t {
        lum,
        red,
        green,
        blue,
        ciel,
        ciea,
        cieb,
        ciec,
        cieh,
        Y,
        Cb,
        Cr,
        hue,
        saturation_hsi,
        saturation_hsv,
        intensity_hsi,
        value_hsv
    };*/

private:
    static constexpr double _1_3 = 1.0 / 3.0;
    static constexpr double _2_3 = 2.0 / 3.0;
    static constexpr double k = 24389.0 / 27.0;
    static constexpr double esp = 216.0 / 24389.0;
    static constexpr double _16_116 = 16.0 / 116.0;
    static constexpr double k_116 = k / 116.0;
    static constexpr double _2pi = 2 * std::numbers::pi;

    inline static const Matrix RGB_YCbCr = Matrix(3, 3, {  0.299,   0.597,   0.114,
                                                          -0.1687, -0.3313,  0.5,
                                                           0.5,    -0.4187, -0.0813});

    inline static const Matrix RGB_XYZ = Matrix(3, 3, { 0.4360747, 0.3850649, 0.1430804,
                                                        0.2225045, 0.7168786, 0.0606169,
                                                        0.0139322, 0.0971045, 0.7141733 });

    inline static const Matrix XYZ_RGB = Matrix(3, 3, { 3.1338561, -1.6168667, -0.4906146,
                                                       -0.9787684,  1.9161415, 0.0334540,
                                                        0.0719453, -0.2289914, 1.4052427 });

    inline static const Matrix D50 = Matrix(3, 1, { 0.96422,
                                      1.0,
                                      0.82521 });

public:
    /*static void RGBtoHSV(double R, double G, double B, double& H, double& S, double& V) {
        double max = math::max(R, math::max(G, B));
        double dm = max - math::min(R, math::min(G, B));

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

    }*/

    static void RGBtoHSV(const Color<double>& color, double& H, double& S, double& V) {
        double R = color.red;
        double G = color.green;
        double B = color.blue;

        double max = math::max(R, math::max(G, B));
        double dm = max - math::min(R, math::min(G, B));

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

    /*static void RGBtoHSVL(double R, double G, double B, double& H, double& S, double& V, double& L) {
        RGBtoHSV(R, G, B, H, S, V);
        L = CIEL(R, G, B);
    }*/

    static void RGBtoHSVL(const Color<double>& color, double& H, double& S, double& V, double& L) {
        RGBtoHSV(color, H, S, V);
        L = CIEL(color);
    }

    /*static void HSVtoRGB(double H, double S, double V, double& R, double& G, double& B) {
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
    }*/

    static Color<double> HSVtoRGB(double H, double S, double V) {
        double R = 0, G = 0, B = 0;

        if (S == 0)
            return { V,V,V };
        

        H *= 6;
        int vi = int(floor(H));
        double v1 = V * (1 - S);
        double v2 = V * (1 - S * (H - vi));
        double v3 = V * (1 - S * (1 - (H - vi)));

        switch (vi) {
        case 0:
            return { V,v3,v1 };

        case 1:
            return { v2,V,v1 };

        case 2:
            return { v1,V,v3 };

        case 3:
            return { v1,v2,V };

        case 4:
            return { v3,v1,V };

        case 5:
            return { V,v1,v2 };

        default: return{ V,V,V };
        }

        return { V,V,V };
    }


    /*static void HSVLtoRGB(double H, double S, double V, double L, double& R, double& G, double& B) {
        HSVtoRGB(H, S, V, R, G, B);
        double a, b, NewL;
        RGBtoCIELab(R, G, B, NewL, a, b);
        CIELabtoRGB(L, a, b, R, G, B);
    }*/

    static Color<double> HSVLtoRGB(double H, double S, double V, double L) {
        Color<double> rgb = HSVtoRGB(H, S, V);
        double a, b, NewL;
        RGBtoCIELab(rgb, NewL, a, b);
        return CIELabtoRGB(L, a, b);
    }

    /*static void RGBtoHSI(double R, double G, double B, double& H, double& S, double& I) {
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
    }*/
    
    static void RGBtoHSI(const Color<double>& color, double& H, double& S, double& I) {
        double R = color.red;
        double G = color.green;
        double B = color.blue;

        double max = math::max(R, math::max(G, B));
        double min = math::min(R, math::min(G, B));
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

    /*static void HSItoRGB(double H, double S, double I, double& R, double& G, double& B) {
        if (S == 0) {
            R = G = B = I;
            return;
        }

        double v2 = (I < 0.5) ? I * (1 + S) : (I + S) - (I * S);
        double v1 = 2 * I - v2;

        R = H_RGB(v1, v2, H + _1_3);
        G = H_RGB(v1, v2, H);
        B = H_RGB(v1, v2, H - _1_3);
    }*/

    static Color<double> HSItoRGB(double H, double S, double I) {
        if (S == 0)
            return { I,I,I };
       

        double v2 = (I < 0.5) ? I * (1 + S) : (I + S) - (I * S);
        double v1 = 2 * I - v2;

        return { H_RGB(v1, v2, H + _1_3) , H_RGB(v1, v2, H), H_RGB(v1, v2, H - _1_3) };
    }

    template<typename T>
    static T HSI_I(const Color<T>& color) {
        return _1_3 * (color.red() + color.green() + color.blue());
    }

    /*static double HSI_I(double R, double G, double B) {
        return _1_3 * (R + G + B);
    }*/

    template<typename T>
    static T HSL_L(const Color<T>& color) {
        T max = math::max(math::max(color.red, color.green), color.blue);
        T min = math::min(math::min(color.red, color.green), color.blue);
        return 0.5 * (max + min);
    }

    template<typename T>
    static T HSI_V(const Color<T>& color) {
        return  math::max(math::max(color.red(), color.green()), color.blue());
    }

    /*static double HSI_V(double R, double G, double B) {
        return  math::max(math::max(R, G), B);
    }*/

private:
    static double H_RGB(double v1, double v2, double H) {
            if (H < 0) H += 1;
            else if (H > 1) H -= 1;

            if (6 * H < 1) return v1 + ((v2 - v1) * 6 * H);
            if (2 * H < 1) return v2;
            if (3 * H < 2) return v1 + (v2 - v1) * ((_2_3 - H) * 6);
            return v1;
        }

public:
    static double sRGBtoLinear(double pixel) {
        return (pixel <= 0.04045) ? pixel / 12.92 : pow((pixel + 0.055) / 1.055, 2.4);
    }

    static double LineartosRGB(double pixel) {
        return (pixel <= 0.0031308) ? 12.92 * pixel : 1.055 * pow(pixel, .41666667) - 0.055;
    }

    /*static void RGBtoXYZ(double R, double G, double B, double& X, double& Y, double& Z) {
        R = sRGBtoLinear(R);
        G = sRGBtoLinear(G);
        B = sRGBtoLinear(B);

        X = Clip(RGB_XYZ(0, 0) * R + RGB_XYZ(0, 1) * G + RGB_XYZ(0, 2) * B);
        Y = Clip(RGB_XYZ(1, 0) * R + RGB_XYZ(1, 1) * G + RGB_XYZ(1, 2) * B);
        Z = Clip(RGB_XYZ(2, 0) * R + RGB_XYZ(2, 1) * G + RGB_XYZ(2, 2) * B);
    }*/

    static void RGBtoXYZ(const Color<float>& color, double& X, double& Y, double& Z) {
        double R = sRGBtoLinear(color.red);
        double G = sRGBtoLinear(color.green);
        double B = sRGBtoLinear(color.blue);

        X = math::clip(RGB_XYZ(0, 0) * R + RGB_XYZ(0, 1) * G + RGB_XYZ(0, 2) * B);
        Y = math::clip(RGB_XYZ(1, 0) * R + RGB_XYZ(1, 1) * G + RGB_XYZ(1, 2) * B);
        Z = math::clip(RGB_XYZ(2, 0) * R + RGB_XYZ(2, 1) * G + RGB_XYZ(2, 2) * B);
    }

    static void RGBtoXYZ(const Color<double>& color, double& X, double& Y, double& Z) {
        double R = sRGBtoLinear(color.red);
        double G = sRGBtoLinear(color.green);
        double B = sRGBtoLinear(color.blue);

        X = math::clip(RGB_XYZ(0, 0) * R + RGB_XYZ(0, 1) * G + RGB_XYZ(0, 2) * B);
        Y = math::clip(RGB_XYZ(1, 0) * R + RGB_XYZ(1, 1) * G + RGB_XYZ(1, 2) * B);
        Z = math::clip(RGB_XYZ(2, 0) * R + RGB_XYZ(2, 1) * G + RGB_XYZ(2, 2) * B);
    }

    /*static void XYZtoRGB(double X, double Y, double Z, double& R, double& G, double& B) {

        R = math::clip(XYZ_RGB(0, 0) * X + XYZ_RGB(0, 1) * Y + XYZ_RGB(0, 2) * Z);
        G = math::clip(XYZ_RGB(1, 0) * X + XYZ_RGB(1, 1) * Y + XYZ_RGB(1, 2) * Z);
        B = math::clip(XYZ_RGB(2, 0) * X + XYZ_RGB(2, 1) * Y + XYZ_RGB(2, 2) * Z);

        R = LineartosRGB(R);
        G = LineartosRGB(G);
        B = LineartosRGB(B);
    }*/

    static Color<float> XYZtoRGBf(double X, double Y, double Z) {

        double R = math::clip(XYZ_RGB(0, 0) * X + XYZ_RGB(0, 1) * Y + XYZ_RGB(0, 2) * Z);
        double G = math::clip(XYZ_RGB(1, 0) * X + XYZ_RGB(1, 1) * Y + XYZ_RGB(1, 2) * Z);
        double B = math::clip(XYZ_RGB(2, 0) * X + XYZ_RGB(2, 1) * Y + XYZ_RGB(2, 2) * Z);

        R = LineartosRGB(R);
        G = LineartosRGB(G);
        B = LineartosRGB(B);

        return Color<float>(R, G, B);
    }

    static Color<double> XYZtoRGB(double X, double Y, double Z) {

        double R = math::clip(XYZ_RGB(0, 0) * X + XYZ_RGB(0, 1) * Y + XYZ_RGB(0, 2) * Z);
        double G = math::clip(XYZ_RGB(1, 0) * X + XYZ_RGB(1, 1) * Y + XYZ_RGB(1, 2) * Z);
        double B = math::clip(XYZ_RGB(2, 0) * X + XYZ_RGB(2, 1) * Y + XYZ_RGB(2, 2) * Z);

        R = LineartosRGB(R);
        G = LineartosRGB(G);
        B = LineartosRGB(B);

        return { R,G,B };
    }

    /*static double CIEL(double R, double G, double B) {
        double Y = RGB_XYZ(1, 0) * sRGBtoLinear(R) + RGB_XYZ(1, 1) * sRGBtoLinear(G) + RGB_XYZ(1, 2) * sRGBtoLinear(B);
        XYZLab(Y);
        return 1.16 * Y - 0.16;
    }*/

    static float CIEL(const Color<float>& color) {
        double Y = RGB_XYZ(1, 0) * sRGBtoLinear(color.red) + RGB_XYZ(1, 1) * sRGBtoLinear(color.green) + RGB_XYZ(1, 2) * sRGBtoLinear(color.blue);
        XYZLab(Y);
        return 1.16 * Y - 0.16;
    }

    static double CIEL(const Color<double>& color) {
        double Y = RGB_XYZ(1, 0) * sRGBtoLinear(color.red) + RGB_XYZ(1, 1) * sRGBtoLinear(color.green) + RGB_XYZ(1, 2) * sRGBtoLinear(color.blue);
        XYZLab(Y);
        return 1.16 * Y - 0.16;
    }

    /*static double CIEa(double R, double G, double B) {
        double X = RGB_XYZ(0, 0) * sRGBtoLinear(R) + RGB_XYZ(0, 1) * sRGBtoLinear(G) + RGB_XYZ(0, 2) * sRGBtoLinear(B);
        double Y = RGB_XYZ(1, 0) * sRGBtoLinear(R) + RGB_XYZ(1, 1) * sRGBtoLinear(G) + RGB_XYZ(1, 2) * sRGBtoLinear(B);

        X /= D50[0];

        XYZLab(X);
        XYZLab(Y);

        return (5 * (X - Y) + 0.9) / 1.8;
    }*/

    static float CIEa(const Color<float>& color) {
        double X = RGB_XYZ(0, 0) * sRGBtoLinear(color.red) + RGB_XYZ(0, 1) * sRGBtoLinear(color.green) + RGB_XYZ(0, 2) * sRGBtoLinear(color.blue);
        double Y = RGB_XYZ(1, 0) * sRGBtoLinear(color.red) + RGB_XYZ(1, 1) * sRGBtoLinear(color.green) + RGB_XYZ(1, 2) * sRGBtoLinear(color.blue);

        X /= D50[0];

        XYZLab(X);
        XYZLab(Y);

        return (5 * (X - Y) + 0.9) / 1.8;
    }

    static double CIEa(const Color<double>& color) {
        double X = RGB_XYZ(0, 0) * sRGBtoLinear(color.red) + RGB_XYZ(0, 1) * sRGBtoLinear(color.green) + RGB_XYZ(0, 2) * sRGBtoLinear(color.blue);
        double Y = RGB_XYZ(1, 0) * sRGBtoLinear(color.red) + RGB_XYZ(1, 1) * sRGBtoLinear(color.green) + RGB_XYZ(1, 2) * sRGBtoLinear(color.blue);

        X /= D50[0];

        XYZLab(X);
        XYZLab(Y);

        return (5 * (X - Y) + 0.9) / 1.8;
    }

    static float CIEb(const Color<float>& color) {
        double Y = RGB_XYZ(1, 0) * sRGBtoLinear(color.red) + RGB_XYZ(1, 1) * sRGBtoLinear(color.green) + RGB_XYZ(1, 2) * sRGBtoLinear(color.blue);
        double Z = RGB_XYZ(2, 0) * sRGBtoLinear(color.red) + RGB_XYZ(2, 1) * sRGBtoLinear(color.green) + RGB_XYZ(2, 2) * sRGBtoLinear(color.blue);

        Z /= D50[2];

        XYZLab(Y);
        XYZLab(Z);

        return (2 * (Y - Z) + 0.9) / 1.8;
    }

    static double CIEb(const Color<double>& color) {
        double Y = RGB_XYZ(1, 0) * sRGBtoLinear(color.red) + RGB_XYZ(1, 1) * sRGBtoLinear(color.green) + RGB_XYZ(1, 2) * sRGBtoLinear(color.blue);
        double Z = RGB_XYZ(2, 0) * sRGBtoLinear(color.red) + RGB_XYZ(2, 1) * sRGBtoLinear(color.green) + RGB_XYZ(2, 2) * sRGBtoLinear(color.blue);

        Z /= D50[2];

        XYZLab(Y);
        XYZLab(Z);

        return (2 * (Y - Z) + 0.9) / 1.8;
    }

private:
    static void XYZLab(double& x) {
        x = (x > esp) ? pow(x, _1_3) : k_116 * x + _16_116;
    }

    static void LabXYZ(double& x) {
        double x3 = x * x * x;
        x = (x3 > esp) ? x3 : (x - _16_116) / k_116;
    }

    /*static void RGBtoCIELab(double R, double G, double B, double& L, double& a, double& b) {
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
    }*/
public:
    static void RGBtoCIELab(const Color<float>& color, double& L, double& a, double& b) {
        double X, Y, Z;
        RGBtoXYZ(color, X, Y, Z);
        X /= D50[0];
        Z /= D50[2];

        XYZLab(X);
        XYZLab(Y);
        XYZLab(Z);

        L = 1.16 * Y - 0.16;
        a = (5 * (X - Y) + 0.9) / 1.8;
        b = (2 * (Y - Z) + 0.9) / 1.8;
    }

    static void RGBtoCIELab(const Color<double>& color, double& L, double& a, double& b) {
        double X, Y, Z;
        RGBtoXYZ(color, X, Y, Z);
        X /= D50[0];
        Z /= D50[2];

        XYZLab(X);
        XYZLab(Y);
        XYZLab(Z);

        L = 1.16 * Y - 0.16;
        a = (5 * (X - Y) + 0.9) / 1.8;
        b = (2 * (Y - Z) + 0.9) / 1.8;
    }

    /*static void CIELabtoRGB(double L, double a, double b, double& R, double& G, double& B) {
        double Y = (L + 0.16) / 1.16;
        double X = (1.8 * a - 0.9) / 5 + Y;
        double Z = Y - (1.8 * b - 0.9) / 2;

        LabXYZ(X);
        LabXYZ(Y);
        LabXYZ(Z);

        X *= D50[0];
        Z *= D50[2];

        XYZtoRGB(X, Y, Z, R, G, B);
    }*/

    static Color<float> CIELabtoRGBf(float L, float a, float b) {
        double Y = (L + 0.16) / 1.16;
        double X = (1.8 * a - 0.9) / 5 + Y;
        double Z = Y - (1.8 * b - 0.9) / 2;

        LabXYZ(X);
        LabXYZ(Y);
        LabXYZ(Z);

        X *= D50[0];
        Z *= D50[2];

        return XYZtoRGBf(X, Y, Z);
    }

    static Color<double> CIELabtoRGB(double L, double a, double b) {
        double Y = (L + 0.16) / 1.16;
        double X = (1.8 * a - 0.9) / 5 + Y;
        double Z = Y - (1.8 * b - 0.9) / 2;

        LabXYZ(X);
        LabXYZ(Y);
        LabXYZ(Z);

        X *= D50[0];
        Z *= D50[2];

        return XYZtoRGB(X, Y, Z);
    }

    /*static void RGBtoCIELch(double R, double G, double B, double& L, double& c, double& h) {
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
    }*/

    static void RGBtoCIELch(const Color<double>& color, double& L, double& c, double& h) {
        double X, Y, Z;
        RGBtoXYZ(color, X, Y, Z);

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

    /*static void CIELchtoRGB(double L, double c, double h, double& R, double& G, double& B) {
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
    }*/

    static Color<double> CIELchtoRGB(double L, double c, double h) {
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

        return XYZtoRGB(X, Y, Z);
    }

    static void RGBtoYCbCr(const Color<uint8_t>& color, uint8_t& Y, uint8_t& Cb, uint8_t& Cr) {

        Y = RGB_YCbCr(0, 0) * color.red + RGB_YCbCr(0, 1) * color.green + RGB_YCbCr(0, 2) * color.blue;
        Cb = 128 - RGB_YCbCr(1, 0) * color.red + RGB_YCbCr(1, 1) * color.green + RGB_YCbCr(1, 2) * color.blue;
        Cr = 128 + RGB_YCbCr(2, 0) * color.red + RGB_YCbCr(2, 1) * color.green + RGB_YCbCr(2, 2) * color.blue;
    }

    static Color<uint8_t> YCbCr(uint8_t Y, uint8_t Cb, uint8_t Cr) {

        uint8_t r = Y + 1.402 * (Cr - 128);
        uint8_t g = Y - 0.34414 * (Cb - 128) - 0.71414 * (Cr - 128);
        uint8_t b = Y + 1.772 * (Cb - 128);

        return { r,g,b };
    }
};
