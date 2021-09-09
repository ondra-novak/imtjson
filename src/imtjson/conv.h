
namespace json {

template<typename T> class ConvValueAs;
template<typename T> class ConvValueFrom;

template<> class ConvValueAs<std::string_view > {
public:
	static std::string_view convert(const Value &v) {
		return v.getString();
	}
};

template<> class ConvValueFrom<std::string_view > {
public:
	static Value convert(const std::string_view &v) {
		return Value(v);
	}
};

template<> class ConvValueAs<UInt > {
public:
	static UInt convert(const Value &v) {
		return v.getUInt();
	}
};

template<> class ConvValueFrom<UInt > {
public:
	static Value convert(UInt v) {
		return Value(v);
	}
};

template<> class ConvValueAs<Int > {
public:
	static Int convert(const Value &v) {
		return v.getInt();
	}
};

template<> class ConvValueFrom<Int > {
public:
	static Value convert(Int v) {
		return Value(v);
	}
};

template<> class ConvValueAs<double> {
public:
	static double convert(const Value &v) {
		return v.getNumber();
	}
};

template<> class ConvValueFrom<double> {
public:
	static Value convert(double v) {
		return Value(v);
	}
};

template<> class ConvValueAs<float> {
public:
	static float convert(const Value &v) {
		return (float)v.getNumber();
	}
};

template<> class ConvValueFrom<float> {
public:
	static Value convert(float v) {
		return Value(v);
	}
};

template<> class ConvValueAs<bool> {
public:
	static bool convert(const Value &v) {
		return v.getBool();
	}
};

template<> class ConvValueFrom<bool> {
public:
	static Value convert(bool v) {
		return Value(v);
	}
};


template<> class ConvValueAs<PValue> {
public:
	static PValue convert(const Value &v) {
		return v.getHandle();
	}
};

template<> class ConvValueFrom<PValue> {
public:
	static Value convert(PValue v) {
		return Value(v);
	}
};

template<> class ConvValueAs<std::string> {
public:
	static std::string convert(const Value &v) {
		return std::string(v.getString());
	}
};
template<> class ConvValueFrom<std::string> {
public:
	static Value convert(const std::string &v) {
		return Value(v);
	}
};

template<typename T> class ConvValueAs<std::vector<T> > {
public:
	static std::vector<T> convert(const Value &v) {
		std::vector<T> out;
		out.reserve(v.size());
		for(auto &&x : v)  out.push_back(x.as<T>());
		return out;
	}
};
template<typename T> class ConvValueFrom<std::vector<T> > {
public:
	static Value convert(const std::vector<T> &v) {
		ValueBuilder bld(v.size());
		for (auto &&k : v) {
			bld.push_back(Value::from<T>(k));
		}
		return bld.commit();
	}
};

}
