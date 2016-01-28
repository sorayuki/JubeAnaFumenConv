#include "FumenReader.h"
#include "MyException.h"

#include <iostream>
#include <sstream>
#include <fstream>

#include <boost/crc.hpp>

using namespace std;
class YubiosiConverter : public FumenParser {
		wstringstream _times;
		wstringstream _keys;

		int _keysCount;

		double _tempo;
		double _beat;
		double _offset;

		int _ShousetsuCount;

		double _currentTime;

		double _speed;
	public:
		YubiosiConverter()
		{
			_beat = 4;
			_tempo = 0;
			_currentTime = 0;
			_keysCount = 0;
			_ShousetsuCount = 0;
			_speed = 1;
			_offset = 0.1;
		}

		void SetSpeed(double speed)
		{
			_speed = speed;
		}

		void SaveToStream(std::wostream& s, const wchar_t* name)
		{
			s << name << endl; //name
			boost::crc_32_type crc;
			crc.process_block(name, name + wcslen(name));
			s << L"key" << crc() << endl; //key
			s << 15000 << endl; //bpm
			s << int(_currentTime * 1000) + 500 << endl; //length
			s << int(_offset / _speed) << endl; //offset
			s << _keysCount << endl; //keys
			s << _times.str();
			s << _keys.str();
		}

		void OnShousetsuData(const Shousetsu& s)
		{
			if (_tempo == 0)
				throw MyException("No tempo info!");

			const std::vector<Haku>& hakus = s.GetHakus();
			
			cout << "Shousetsu: " << _ShousetsuCount;
			cout << " hakus:" << hakus.size();
			int nKeys = 0;
			
			for (auto i = hakus.cbegin(), e = hakus.cend(); i != e; ++i) {
				double hakuNum = i->GetNum();
				double hakuTime = 60 * hakuNum / _tempo / _speed;
				HakuKeys::KeyCollections keys = i->GetKeys().GetKeys();
				for (auto k = keys.cbegin(), ke = keys.cend(); k != ke; ++k) {
					_keys << ( 1 << (k->first * 4 + k->second) ) << endl;
					_times << int( (hakuTime + _currentTime) * 1000 ) << endl;
					++_keysCount;
					++nKeys;
				}
			}
			_currentTime += 60 * _beat / _tempo / _speed;

			cout << " keys:" << nKeys << endl;

			++_ShousetsuCount;
		}

		void OnFumenInfoData(const FumenInfo& f)
		{
			if (f.type == FumenInfo::INFOTYPE_BEATS)
				_beat = boost::any_cast<double>(f.value);
			else if (f.type == FumenInfo::INFOTYPE_OFFSETR)
				_offset += boost::any_cast<double>(f.value);
			else if (f.type == FumenInfo::INFOTYPE_OFFSETO)
				_offset = boost::any_cast<double>(f.value);
			else if (f.type == FumenInfo::INFOTYPE_TEMPO)
				_tempo = boost::any_cast<double>(f.value);
		}
};

#include <locale>
#include <regex>
#include <codecvt>

int wmain(int argc, wchar_t* argv[])
{
	static locale jpLoc("japanese", std::locale::ctype);
	static locale defLoc("", std::locale::ctype);
	static locale utf8Loc(defLoc, new std::codecvt_utf8<wchar_t>());

	if (argc != 3) {
		cerr << "run this program with 2 parameters: input and output filename" << endl;
		return 1;
	}
	
	try {
		//Fumen2XML fp;
		YubiosiConverter fp;

		//fp.SetSpeed(0.9);
		
		static const wregex fnReg(L"([^\\\\]*?\\\\)?(.+?)\\.[Tt][Xx][Tt]");

		wcmatch fnMatch;

		wstring name;
		if (regex_match(argv[2], fnMatch, fnReg)) {
			name = fnMatch[2].str();
		} else {
			name = argv[2];
		}

		fstream fs(argv[1], std::ios::in);

		vector<wstring> lines;
		//read lines
		while(!fs.eof()) {
			string line;
			getline(fs, line);

			if (line.length() > 3 && line[0] == 'm')
				lines.push_back(EncodingConv(line, defLoc));
			else
				lines.push_back(EncodingConv(line, jpLoc));
		}

		fp.LoadString(lines);
		fs.close();

		wfstream fs2(argv[2], std::ios::out);
		//fs2.imbue(jpLoc);
		fs2.imbue(utf8Loc);

		fp.SaveToStream( fs2, name.c_str() );
		return 0;
	} catch (MyException& e) {
		cerr << e.what() << endl;
		return 1;
	} catch (exception& e) {
		cerr << e.what() << endl;
		return 2;
	}
}
