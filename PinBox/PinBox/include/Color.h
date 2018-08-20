#ifndef _PP_COLOR_H_
#define _PP_COLOR_H_

#include <functional>

#define M_MIN(a,b) (((a) < (b)) ? (a) : (b))
#define M_MAX(a,b) (((a) > (b)) ? (a) : (b))

#define M_MIN3(a,b,c) (((a) < (b)) ? (((a) < (c)) ? (a) : (c)) : (((b) < (c)) ? (b) : (c)))
#define M_MAX3(a,b,c) (((a) > (b)) ? (((a) > (c)) ? (a) : (c)) : (((b) > (c)) ? (b) : (c)))

//TODO: port this
// https://github.com/Qix-/color-convert/blob/master/conversions.js
// and this https://github.com/Qix-/color/blob/master/index.js#L298

struct Color;
#define RGB(r,g,b) Color{r, g, b, 255}
#define RGBA(r,g,b,a) Color{r, g, b, a}
#define TRANSPARENT Color{0, 0, 0, 0}

typedef struct Color
{
	Color() : r(255), g(255), b(255), a(255) {};
	Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) : r(_r), g(_g), b(_b), a(_a) {};
	uint8_t r, g, b, a;
	float h, l, s, v;
	u32 toU32() const { return (((a) & 0xFF) << 24) | (((b) & 0xFF) << 16) | (((g) & 0xFF) << 8) | (((r) & 0xFF) << 0); }
	// type conversions
	inline void rgb2hsl()
	{
		float _r = r / 255.f, _g = g / 255.f, _b = b / 255.f;
		float vMin = M_MIN3(r, g, b);
		float vMax = M_MAX3(r, g, b);
		float delta = vMax - vMin;
		if (vMax == vMin) h = 0;
		else if (_r == vMax) h = (_g - _b) / delta;
		else if (_g == vMax) h = 2 + (_b - _r) / delta;
		else if (_b == vMax) h = 4 + (_r - _g) / delta;
		h = M_MIN(h * 60.f, 36.f);
		if (h < 0) h += 360.f;
		l = (vMin + vMax) / 2.0f;
		if (vMax == vMin) s = 0;
		else if (l <= 0.5f) s = delta / (vMax + vMin);
		else s = delta / (2 - vMax - vMin);
	}
	inline void hsl2rgb()
	{
		float _h = h / 360.f, _s = s / 360.f, _l = l / 360.f;
		float t1 = 0, t2 = 0, t3 = 0, val = 0;
		if (_s == 0) {
			val = _l * 255.f;
			r = val, g = val, b = val;
		}else
		{
			if (_l < 0.5f) t2 = _l * (1 + _s);
			else t2 = _l + _s - _l * _s;
			t1 = 2 * _l - t2;
			std::function<float(int i)> get = std::function<float(int i)>([&](int i) {
				t3 = _h + 1.f / 3.f * -(i - 1);
				if (t3 < 0) t3++;
				if (t3 > 1) t3--;
				if (6.f * t3 < 1) val = t1 + (t2 - t1) * 6.f * t3;
				else if (2.f * t3 < 1.f) val = t2;
				else if (3.f * t3 < 2.f) val = t1 + (t2 - t1) * (2.f / 3.f - t3) * 6.f;
				else val = t1;
				return val * 255.f;
			});
			r = get(0);
			g = get(1);
			b = get(2);
		}
	}
	inline void rgb2hsv()
	{
		float rd, gd, bd;
		float _r = r / 255.f, _g = g / 255.f, _b = b / 255.f;
		v = M_MAX3(_r, _g, _b);
		float diff = v - M_MIN3(_r, _g, _b);
		std::function<float(float c)> diffc = std::function<float(float c)>([&](float c) {
			return (v - c) / 6.f / diff + 1.f / 2.f;
		});
		if (diff == 0.f) h = s = 0.f;
		else
		{
			s = diff / v;
			rd = diffc(_r);
			gd = diffc(_g);
			bd = diffc(_b);
			if (_r == v) h = bd - gd;
			else if (_g == v) h = (1.f / 3.f) + rd - bd;
			else if (_b == v) h = (2.f / 3.f) + gd - rd;
			if (h < 0) h += 1;
			else if (h > 1) h -= 1;
		}
		h *= 360; s *= 100; v *= 100;
	}
	inline void hsv2rgb()
	{
		float _h = h / 60.f, _s = s / 100.f, _v = v / 100.f;
		int hi = (int)floor(_h) % 6;
		uint8_t f = _h - floor(_h);
		uint8_t p = 255.f * _v * (1.f - _s);
		uint8_t q = 255.f * _v * (1.f - (_s * f));
		uint8_t t = 255.f * _v * (1 - (_s * (1 - f)));
		_v *= 255.f;
		switch (hi) {
		case 0: r = _v; g = t; b = p; break;
		case 1: r = q; g = _v; b = p; break;
		case 2: r = p; g = _v; b = t; break;
		case 3: r = p; g = q; b = _v; break;
		case 4: r = t; g = p; b = _v; break;
		case 5: r = _v; g = p; b = q; break;
		}
	}

#define WARP_HSL(func)  rgb2hsl(); func; hsl2rgb();
#define WARP_HSV(func)  rgb2hsv(); func; hsv2rgb();

	// funcs
	inline float luminosity()
	{
		float lr = ((float)r / 255.0f); lr = lr <= 0.03928f ? lr / 12.92f : powf((lr + 0.055f) / 1.055f, 2.4f);
		float lg = ((float)g / 255.0f); lg = lg <= 0.03928f ? lg / 12.92f : powf((lg + 0.055f) / 1.055f, 2.4f);
		float lb = ((float)b / 255.0f); lb = lb <= 0.03928f ? lb / 12.92f : powf((lb + 0.055f) / 1.055f, 2.4f);
		return  0.2126 * lr + 0.7152 * lg + 0.0722 * lb;
	}
	inline float contrast(Color c)
	{
		float l1 = luminosity(), l2 = c.luminosity();
		return l1 > l2 ? (l1 + 0.05f) / (l2 + 0.05f) : (l2 + 0.05f) / (l1 + 0.05f);
	}
	inline int level(Color c)
	{
		float cRatio = contrast(c);
		if (cRatio >= 7.1f) return 3;
		return (cRatio >= 4.5) ? 2 : 1;
	}
	inline bool isDark() { return (float)(r * 299 + g * 587 + b * 114) / 1000.0f < 128.0f; }
	inline bool isLight() { return !isDark(); }
	inline Color negate() { return RGB(255-r, 255-g, 255-b); }
	inline void lighten(float ratio) { WARP_HSL(l += l * ratio) }
	inline void darken(float ratio) { WARP_HSL(l -= l * ratio) }
	inline void saturate(float ratio) { WARP_HSL(s += s * ratio) }
	inline void desaturate(float ratio) { WARP_HSL(s -= s * ratio) }
	inline Color grayscale() { float v = r * 0.3f + g * 0.59f + b * 0.11f; return RGB(v, v, v); }
	inline void rorate(float degrees) { WARP_HSL( h = (int)(h + degrees) % 360; h = h < 0 ? 360 + h : h; ) }
	inline Color mix(Color mixin, float weight)
	{
		float w = 2.f * weight - 1;
		uint8_t _a = a - mixin.a;
		float w1 = (((w * _a == -1) ? w : (w + _a) / (1.f + w * _a)) + 1.f) / 2.f;
		float w2 = 1 - w1;
		return RGBA(w1 * r + w2 * mixin.r, w1 * g + w2 * mixin.g, w1 * b + w2 * mixin.b, a * weight + mixin.a * (1 - weight));
	}
};


#endif