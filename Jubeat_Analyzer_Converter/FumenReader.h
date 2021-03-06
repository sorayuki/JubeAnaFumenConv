#pragma once

#include <boost/utility.hpp>
#include <vector>
#include <string>
#include <utility>
#include <boost/any.hpp>
#include <cstdint>
#include <locale>

//haku: 拍
class Haku;
class HakuKeys;

class HakuKeys {
	public:
		typedef std::pair<int, int> KeyPosition;
		typedef std::vector<KeyPosition> KeyCollections;

		friend class Haku;
	private:
		std::uint16_t _keys; //row, column

		bool GetKey(int r, int c) const;
		static int GetBitMask(int r, int c);

		HakuKeys(); //only for class Haku
	public:
		void SetKey(int r, int c);
		void Clear();
		KeyCollections GetKeys() const;
		bool IsEmpty() const;

		HakuKeys(const HakuKeys& h);
		HakuKeys& operator = (const HakuKeys& h);
};

class Haku {
		HakuKeys _keys;
		double _num;
	public:
		Haku(double n);
		HakuKeys& GetKeys();
		const HakuKeys& GetKeys() const;
		double GetNum() const;
		bool IsEmpty() const;

		Haku(const Haku& h);
		Haku& operator = (const Haku& h);
};

class Shousetsu {
	protected:
		std::vector<Haku> _hakus;
	public:
		Shousetsu();
		
		Shousetsu(const Shousetsu&);
		Shousetsu& operator=(const Shousetsu&);

		Shousetsu(Shousetsu&& s); //movable
		Shousetsu& operator=(Shousetsu&&);

		void ApppendHaku(const Haku& h);
		const std::vector<Haku>& GetHakus() const;
};

struct FumenInfo {
	enum { INFOTYPE_BEATS = 1, INFOTYPE_TEMPO, INFOTYPE_OFFSETR, INFOTYPE_OFFSETO, INFOTYPE_MUSICFILE };
	int type;
	boost::any value;
};

class FumenParser : boost::noncopyable {
	protected:
		virtual void OnShousetsuData(const Shousetsu&) = 0;
		virtual void OnFumenInfoData(const FumenInfo&) = 0;
	public:
		FumenParser();
		void LoadString(std::vector<std::wstring>& lines);
};

class FumenParser_TimeCallback : public FumenParser {
	protected:
		double _tempo;
		double _beat;
		double _offset;

		double _currentTime;

		double _speed;

		bool _isFirstMarker;
	public:
		FumenParser_TimeCallback();

		void SetSpeed(double speed);

	protected:
		virtual void OnTimeCallback(double dTime, const HakuKeys::KeyCollections& keys, bool newShousetsu) = 0;

		void OnShousetsuData(const Shousetsu& s);

		void OnFumenInfoData(const FumenInfo& f);
};

std::wstring EncodingConv(const std::string& input, const std::locale& loc);
