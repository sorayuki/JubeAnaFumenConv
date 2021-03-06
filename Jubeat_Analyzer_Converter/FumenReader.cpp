#include "FumenReader.h"
#include <algorithm>

#include "MyException.h"

//-------------------------------------------------------------------
//impl for hakukeys
//-------------------------------------------------------------------

HakuKeys::HakuKeys()
{
	Clear();
}

HakuKeys::HakuKeys(const HakuKeys& h)
{
	*this = h;
}

HakuKeys& HakuKeys::operator= (const HakuKeys& h)
{
	_keys = h._keys;

	return *this;
}

int HakuKeys::GetBitMask(int r, int c)
{
	int p = r * 4 + c;
	return 1 << p;
}

bool HakuKeys::GetKey(int r, int c) const
{
	if (r > 4 || c < 0 || r > 4 || c < 0)
		throw MyException("Incorrect position for a Key!");

	return (GetBitMask(r, c) & _keys) != 0;
}

void HakuKeys::SetKey(int r, int c)
{
	if (r > 4 || c < 0 || r > 4 || c < 0)
		throw MyException("Incorrect position for a Key!");

	_keys |= GetBitMask(r, c);
}

void HakuKeys::Clear()
{
	_keys = 0;
}

HakuKeys::KeyCollections HakuKeys::GetKeys() const
{
	KeyCollections r;
	for(int i = 0; i < 4; ++i)
		for(int j = 0; j < 4; ++j)
			if (GetKey(i, j))
				r.push_back(std::make_pair(i, j));

	return std::move(r);
}

bool HakuKeys::IsEmpty() const
{
	return _keys == 0;
}

//-------------------------------------------------------------------
//impl for haku
//-------------------------------------------------------------------

Haku::Haku(double n) : _num(n)
{
}

Haku::Haku(const Haku& h)
{
	*this = h;
}

Haku& Haku::operator= (const Haku& h)
{
	_num = h._num;
	_keys = h._keys;

	return *this;
}

HakuKeys& Haku::GetKeys()
{
	return _keys;
}

const HakuKeys& Haku::GetKeys() const
{
	return _keys;
}

double Haku::GetNum() const
{
	return _num;
}

bool Haku::IsEmpty() const
{
	return _keys.IsEmpty();
}

//-------------------------------------------------------------------
//impl for shousetsu
//-------------------------------------------------------------------
Shousetsu::Shousetsu()
{
}

Shousetsu::Shousetsu(Shousetsu&& s)
{
	*this = std::move(s);
}

Shousetsu& Shousetsu::operator=(Shousetsu&& s)
{
	_hakus = std::move(s._hakus);
	return *this;
}

Shousetsu::Shousetsu(const Shousetsu& s)
{
	*this = s;
}

Shousetsu& Shousetsu::operator=(const Shousetsu& s)
{
	_hakus = s._hakus;
	return *this;
}

void Shousetsu::ApppendHaku(const Haku& h)
{
	_hakus.push_back(h);
}

const std::vector<Haku>& Shousetsu::GetHakus() const
{
	return _hakus;
}

//-------------------------------------------------------------------
//impl for fumenparser
//-------------------------------------------------------------------
#include <regex>

FumenParser::FumenParser()
{
}

static bool CheckFor2Column(const std::vector<std::wstring>& lines)
{
	using namespace std;

	static const wregex dblColChecker(L"\\|.{4}\\|");
	for(auto i = lines.cbegin(), e = lines.cend(); i != e; ++i)
	{
		wsmatch mat;
		if ( regex_search(*i, mat, dblColChecker) == true )
		{
			return true;
		}
	}
	return false;
}

#include <array>
#include <functional>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

static bool ParseFumenInfo(const std::wstring& line, FumenInfo& fi)
{
	using namespace std;
	using boost::lexical_cast;

	static const wregex tempoChecker(L"t *= *(\\d+(\\.\\d+)?)");
	static const wregex offsetChecker(L"r *= *(-?\\d+(\\.\\d+)?)");
	static const wregex firstMarkerTimeChecker(L"o *= *(-?\\d+(\\.\\d+)?)");
	static const wregex beatChecker(L"b *= *(\\d+(\\.\\d+)?)");
	static const wregex musicChecker(L"m *= *\"(.*?)\"");

	wsmatch match;
	if (regex_match(line, match, tempoChecker) == true) {
		fi.type = FumenInfo::INFOTYPE_TEMPO;
		fi.value = lexical_cast<double>(match[1].str());
		return true;
	} else if (regex_match(line, match, offsetChecker) == true) {
		fi.type = FumenInfo::INFOTYPE_OFFSETR;
		fi.value = lexical_cast<double>(match[1].str());
		return true;
	} else if (regex_match(line, match, beatChecker) == true) {
		fi.type = FumenInfo::INFOTYPE_BEATS;
		fi.value = lexical_cast<double>(match[1].str());
		return true;
	} else if (regex_match(line, match, musicChecker) == true) {
		fi.type = FumenInfo::INFOTYPE_MUSICFILE;
		fi.value = wstring(match[1].str());
		return true;
	} else if (regex_match(line, match, firstMarkerTimeChecker) == true) {
		fi.type = FumenInfo::INFOTYPE_OFFSETO;
		fi.value = lexical_cast<double>(match[1].str());
		return true;
	} else
		return false;

	throw MyException("Internal Error: function ParseFumenInfo is incorrect.");
}

#include <unordered_map>
#include <algorithm>
typedef std::unordered_map<wchar_t, double> HakuMetaType;

static void RestoreDefaultSingleColumnHakuMetas(HakuMetaType& metas)
{
	metas.clear();

	const wchar_t* keys = L"①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯";
	for(int i = 0; i < 16; ++i)
		metas[keys[i]] = static_cast<double>(i) / 4;
}

//conver every bar's raw information into internal data structure
//把每个小节的原始信息转成内部数据结构
static Shousetsu ConvertToShousetsu(const std::vector<const std::wstring*>& rawLines, const HakuMetaType& metas)
{
	Shousetsu r;
	int nLineCount = rawLines.size();

	std::vector<HakuMetaType::const_iterator> sortedHaku;
	for (auto i = metas.cbegin(), e = metas.cend(); i != e; ++i)
		sortedHaku.push_back(i);
	std::sort(sortedHaku.begin(), sortedHaku.end(),
		[](const HakuMetaType::const_iterator& a, const HakuMetaType::const_iterator& b)-> bool {
			if (a->second < b->second) return true;
			else return false;
		});

	for (auto i = sortedHaku.cbegin(), e = sortedHaku.cend(); i != e; ++i) {
        //对每个部分，遍历所有“带圈数字”，即节拍
        //iterator all circled number for all part. a.k.a. beat
		Haku h((*i)->second);

		for (int nLine = 0; nLine < nLineCount; ++nLine) { 
            //“部分”是一行行来
            //"part" is processed line by line
			int nLineInPart = nLine % 4;
			const std::wstring& sLine = *rawLines[nLine];

			//在当前行查找“带圈数字”（节拍），一行里面所有的都要
			//只找前四个字符，因为两列的meta信息也……
            //look for all beats (circled number) in current line
            //only process first 4 char.
			int column;
			int nSpos = 0;
			while ( (column = sLine.find((*i)->first, nSpos) ) != sLine.npos && column < 4) {
				h.GetKeys().SetKey(nLineInPart, column);
				nSpos = column + 1;
			}
		}

		if (!h.IsEmpty())
			r.ApppendHaku(h);
	}

	return std::move(r);
}

//下面两个分割小节用的函数中，公共检查部分，返回true表示已处理
//the common part of the following 2 functions for spltting. returning true means processed
static bool SplitShousetsu_CommonCheck(const std::wstring& line, HakuMetaType& hakumetas)
{
	using namespace std;
	using boost::lexical_cast;

	static const wregex hakuMetaChecker(L"\\*(.) *: *(\\d+(\\.\\d+)?).*");

	//ignore empty line
	if (line.empty()) return true;

	//ignore comment
	if (line[0] == L'/' && line[1] == L'/') return true;

	//ignore #memo
	if (line == L"#memo") return true;

	//check if it's haku meta
	wsmatch hakuMetaMatch;
	if (regex_match(line, hakuMetaMatch, hakuMetaChecker) == true) {
		hakumetas[hakuMetaMatch[1].str()[0]] = lexical_cast<double>(hakuMetaMatch[2].str());
		return true;
	}

	return false;
}

//把行分割成每个小节、或者控制信息，同时负责分析override信息，然后调用回调函数
//split line into bars or control information, and parse override control command. and then call the callback function
static void ParseLines_1(const std::vector<std::wstring>& lines,
	std::function<void (const Shousetsu&)> shousetsuCallback,
	std::function<void (const FumenInfo&)> infoCallback)
{
	using namespace std;
	using boost::lexical_cast;

	vector<const wstring*> shousetsu;
	HakuMetaType hakumetas;

	RestoreDefaultSingleColumnHakuMetas(hakumetas);

	for(int i = 0, e = lines.size(); i < e; ++i)
	{
		const wstring& line = lines[i];

		//公共部分，如果处理成功就跳过
        //common processing
		if (SplitShousetsu_CommonCheck(line, hakumetas)) continue;

		//检查是不是谱面信息
        //check if it's fumen information
		FumenInfo fi;
		if( ParseFumenInfo(line, fi) ) {
			infoCallback(fi);
			continue;
		}

		//check if it's shousetsu split
		if ( line[0] == L'-' && line[1] == L'-' ) {

			//check shousetsu lines
			if (shousetsu.size() % 4 != 0)
				throw MyException( (boost::format("Parse Fumen Error on line %d") % i).str().c_str(), true );

			shousetsuCallback(ConvertToShousetsu(shousetsu, hakumetas));

			//clear environment
			shousetsu.clear();
		} else
			shousetsu.push_back(&line);
	}

	//最后一个小节，没有线
    //last bar may come without line (------)
	if (shousetsu.size() % 4 != 0)
		throw MyException("Parse Fumen Error on last lines");
	if (shousetsu.size() != 0)
		shousetsuCallback(ConvertToShousetsu(shousetsu, hakumetas));
}

static void ConvertToHakuMeta(const std::vector< std::pair<const wchar_t*, const wchar_t*> >& rawMeta, HakuMetaType& metas)
{
	for (int i = 0, ie = rawMeta.size(); i < ie; ++i) {
		for (const wchar_t *js = rawMeta[i].first, *j = rawMeta[i].first, *je = rawMeta[i].second; j < je; ++j ) {
			metas[*j] = i + double(j - js) / 4;
		}
	}
}

//把行分割成每个小节、或者控制信息，同时负责分析override信息，然后调用回调函数，两行
//split line into bars or control information. parse override command as well.
static void ParseLines_2(const std::vector<std::wstring>& lines,
	std::function<void (const Shousetsu&)> shousetsuCallback,
	std::function<void (const FumenInfo&)> infoCallback)
{
	using namespace std;
	using boost::lexical_cast;

	vector<const wstring*> rawshousetsu;
	
	HakuMetaType hakumetas;

	//当前已收集的原始meta信息
    //all meta information collected
	vector< pair<const wchar_t*, const wchar_t*> > rawhakumetas;

	//每小节节拍数，有用
    //beats in a bar
	double beat = 4;

	static const wregex hakumetaChecker(L"\\|(.{1,4})\\|");
	static const wregex numberOnlyLine(L" *\\d+ *");

	for (int i = 0, e = lines.size(); i < e; ++i) {
		const wstring& line = lines[i];

		//公共部分，如果处理成功就跳过
        //common processing
		if (SplitShousetsu_CommonCheck(line, hakumetas)) continue;

		//检查是不是谱面信息
        //check if it's fumen information
		FumenInfo fi;
		if( ParseFumenInfo(line, fi) ) {
			infoCallback(fi);
			//如果节拍改变……
            //if beat changes
			if (fi.type == FumenInfo::INFOTYPE_BEATS)
				beat = boost::any_cast<double>(fi.value);
			continue;
		}

		//检查是不是小节线
        //check if it's bar splitter line (--------)
		if ( line[0] == L'-' && line[1] == L'-' ) {
			//注意！！从下面复制的代码
            //NOTICE: CODE COPIED FROM BELOW
			//转换meta信息
            //convert meta information
			ConvertToHakuMeta(rawhakumetas, hakumetas);
			//转换节拍信息
            //convert beat information
			shousetsuCallback(ConvertToShousetsu(rawshousetsu, hakumetas));

			//clear environment
			rawhakumetas.clear();
			rawshousetsu.clear();

			continue;
		}

		//抓取meta信息
        //extract meta information
		wsmatch metaMatch;
		if ( regex_search(line, metaMatch, hakumetaChecker) == true )
			rawhakumetas.push_back( make_pair(&(*metaMatch[1].first), &(*metaMatch[1].second)) );

		//忽略纯数字的行
        //ignore line with only numbers
		if ( regex_match(line, numberOnlyLine) == true )
			continue;

		//加入当前行
        //push current line to buffer
		rawshousetsu.push_back( &line );

		//meta原始行不能大于beat数
        //original line for meta cannot longer than beats
		if (rawhakumetas.size() - 0.01 >= (beat + 1))
			throw MyException( (boost::format("Incorrect the second columns! line %d") % i).str().c_str(), true );

		//判断是否小节已结束
        //check if it's end of a bar
		if ( rawhakumetas.size() + 0.01 >= beat && rawhakumetas.size() - 0.01 < (beat + 1) && rawshousetsu.size() % 4 == 0) {
			//注意！！！上面有一段代码从这里复制的
            //NOTICE: SOME CODE ABOVE COPIED FROM HERE
			//转换meta信息
            //convert meta informatino
			ConvertToHakuMeta(rawhakumetas, hakumetas);
			//转换节拍信息
            //convert beat information
			shousetsuCallback(ConvertToShousetsu(rawshousetsu, hakumetas));

			//clear environment
			rawhakumetas.clear();
			rawshousetsu.clear();
		}
	}
}

void FumenParser::LoadString(std::vector<std::wstring>& lines)
{
	using namespace std::placeholders;

	//remove comments and spaces
	for (auto i = lines.begin(), ie = lines.end(); i != ie; ++i) {
		std::wstring::size_type commentIndex;

		if ( i->length() != 0 && (*i)[0] == L'#' ) {
			i->clear();
		} else {
			//find comment index
			if ( (commentIndex = i->find(L"//")) == std::wstring::npos) {
				commentIndex = i->length();
			}

			//find first non space
			std::wstring::size_type nsHeadIndex = 0;
			for(std::wstring::size_type ed = i->length(); nsHeadIndex < ed; ++nsHeadIndex)
				if ( (*i)[nsHeadIndex] != ' ' ) break;

			//find last non space
			std::wstring::size_type nsTailIndex = commentIndex;
			for(std::wstring::size_type st = nsHeadIndex; nsTailIndex > nsHeadIndex; --nsTailIndex)
				if ( (*i)[nsTailIndex - 1] != ' ') break;

			*i = i->substr(nsHeadIndex, nsTailIndex - nsHeadIndex);
		}
	}

	if (CheckFor2Column(lines) == true)
		ParseLines_2(lines, 
			std::bind(&FumenParser::OnShousetsuData, this, _1),
			std::bind(&FumenParser::OnFumenInfoData, this, _1));
	else
		ParseLines_1(lines,
			std::bind(&FumenParser::OnShousetsuData, this, _1),
			std::bind(&FumenParser::OnFumenInfoData, this, _1));
}


//-------------------------------------------------------------------
//impl for fumen parser time callback
//-------------------------------------------------------------------
FumenParser_TimeCallback::FumenParser_TimeCallback()
{
	_beat = 4;
	_tempo = 0;
	//(諸悪の根源)
	_currentTime = 0.1;
	_offset = 0;
	_isFirstMarker = true;
	_speed = 1;
}

void FumenParser_TimeCallback::SetSpeed(double speed)
{
	_speed = speed;
	_currentTime = 0.1 / speed;
}

void FumenParser_TimeCallback::OnShousetsuData(const Shousetsu& s)
{
	if (_tempo == 0)
		throw MyException("No tempo info!");

	const std::vector<Haku>& hakus = s.GetHakus();

	bool newShousetsu = true;

	for (auto i = hakus.cbegin(), e = hakus.cend(); i != e; ++i) {
		double hakuNum = i->GetNum();
		double hakuTime = 60 * hakuNum / _tempo / _speed;
		HakuKeys::KeyCollections keys = i->GetKeys().GetKeys();

		OnTimeCallback(hakuTime + _currentTime + _offset / 1000.0, keys, newShousetsu);
		newShousetsu = false;
	}
	_currentTime += 60 * _beat / _tempo / _speed;
}

void FumenParser_TimeCallback::OnFumenInfoData(const FumenInfo& f)
{
	if (f.type == FumenInfo::INFOTYPE_BEATS)
		_beat = boost::any_cast<double>(f.value);
	else if (f.type == FumenInfo::INFOTYPE_OFFSETR)
		_offset = boost::any_cast<double>(f.value);
	else if (f.type == FumenInfo::INFOTYPE_TEMPO)
		_tempo = boost::any_cast<double>(f.value);
	else if (f.type == FumenInfo::INFOTYPE_OFFSETO)
		//in jubeat analyzer's source code, o= makes currenttime =
		//and r= makes currenttime +=
		_offset = boost::any_cast<double>(f.value) - _currentTime * 1000;
}


//-------------------------------------------------------------------
//impl for free functions
//-------------------------------------------------------------------

#include <locale>
#include <codecvt>

std::wstring EncodingConv(const std::string& input, const std::locale& loc) {
	typedef std::codecvt<wchar_t, char, std::mbstate_t> MyCodeCvt;

	std::wstring r;
	int inpLen = input.length();
	if (inpLen == 0) return std::move(r);

	r.resize(inpLen);
	const char* inpPos;
	wchar_t* outPos;
	wchar_t* rBegin = &r[0];
	const char* sBegin = input.c_str();

	std::mbstate_t inState = 0;
	std::use_facet<MyCodeCvt>(loc).in(inState,
		sBegin, sBegin + inpLen, inpPos,
		rBegin, rBegin + inpLen, outPos);

	r.resize(outPos - rBegin);

	return std::move(r);
}
