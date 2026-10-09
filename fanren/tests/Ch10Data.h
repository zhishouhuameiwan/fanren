#pragma once

#include <array>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "../vendor/json.hpp"

namespace fanren::test::ch10 {
namespace fs = std::filesystem;
using Json = nlohmann::json;

inline fs::path root() {
#ifdef FANREN_CH10_ASSET_ROOT
    return FANREN_CH10_ASSET_ROOT;
#else
    return fs::absolute(fs::path(__FILE__)).parent_path().parent_path();
#endif
}

inline std::string read(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot read " + path.string());
    std::ostringstream out;
    out << input.rdbuf();
    return out.str();
}

inline Json json(const fs::path& path) { return Json::parse(read(path)); }

inline constexpr std::array<const char*, 7> kMaps{
    "ch10_gudao", "ch10_haichuan", "ch10_kuixing", "ch10_xiaohuan",
    "ch10_tiandujie", "ch10_jinhai", "ch10_haiyuandao"};
inline constexpr std::array<const char*, 4> kBattles{
    "b10_gujia_bidou", "b10_liuliandian_weisha", "b10_gu_zhanglao", "b10_zhifadui"};
inline constexpr std::array<const char*, 3> kLostMagics{
    "magic_ji_qingning", "magic_ji_wulongduo", "magic_ji_baizhudao"};
inline const std::set<std::string> kIncomingMagics{
    "magic_huodan_shu", "magic_yufeng_jue", "magic_tianyan_shu", "magic_liusha_shu",
    "magic_bingdong_shu", "magic_ji_jinfu", "magic_ji_qingjiao", "magic_ji_qingning",
    "magic_qingyuan_jianmang", "magic_ji_qinghuozhang", "magic_ji_wulongduo",
    "magic_ji_xiaodao", "magic_ji_hongxianzhen", "magic_ji_baizhudao"};
inline constexpr std::array<const char*, 28> kAccounts{
    "material_lingshi", "material_lingshi_zhong", "pill_zhuji_dan", "pill_dingyan_dan",
    "pill_zhenyuan_dan", "pill_jiangchen_dan", "material_xuelingshui", "material_tianhuoye",
    "pill_xuening_wuxing_dan", "story_jin_kuloutou", "story_hunyuan_bo", "talisman_jinjian_fubao",
    "talisman_lanjiao_fu", "story_tulijue", "material_yaodan_wuji", "story_shijin_chong",
    "story_qingning_jing", "story_wulong_duo", "story_baizhu_feidao", "story_lvse_yupai",
    "story_lanse_yupei", "story_xiaohuan_yujian", "story_haiyu_tu", "story_luanxing_danfang",
    "story_dandao_pingjian", "herb_zishen_cao", "herb_xuehong_zhi", "story_danuoyi_ling"};

struct Hook {
    int node;
    const char* map;
    const char* object;
    const char* script;
    const char* guard;
    const char* done;
    bool enter = false;
    bool once = true;
    int days = 0;
};

// Independent oracle: design 3.1/3.3, not the shipped objective table.
inline constexpr std::array<Hook, 32> kStory{{
    {1,"ch10_gudao","trigger_huizhen","huizhen","ch09.done","ch10.huizhen"},
    {2,"ch10_gudao","trigger_jushi","jushi","ch10.huizhen","ch10.jushi",true},
    {3,"ch10_gudao","trigger_wanghai","wanghai","ch10.jushi","ch10.likai",false,true,1},
    {4,"ch10_haichuan","trigger_chuantou","chuantou","ch10.likai","ch10.guyu"},
    {5,"ch10_haichuan","trigger_yetan","yetan","ch10.guyu","ch10.yetan",false,true,1},
    {6,"ch10_haichuan","trigger_kaoan","kaoan","ch10.yetan","ch10.kaoan"},
    {7,"ch10_kuixing","trigger_dengji","dengji","ch10.kaoan","ch10.dengji"},
    {8,"ch10_kuixing","trigger_muwu","muwu","ch10.dengji","ch10.muwu",false,true,32},
    {9,"ch10_kuixing","trigger_gongdian","gongdian","ch10.muwu","ch10.chouqian"},
    {10,"ch10_kuixing","trigger_leitai","leitai","ch10.chouqian","ch10.leitai"},
    {11,"ch10_kuixing","trigger_dengxian","dengxian","ch10.leitai","ch10.dengxian",false,true,3},
    {12,"ch10_kuixing","trigger_xuandi","xuandi","ch10.dengxian","ch10.xuandi",false,true,2},
    {13,"ch10_xiaohuan","trigger_matou","matou","ch10.xuandi","ch10.zhenzhang"},
    {14,"ch10_xiaohuan","trigger_kaifu","kaifu","ch10.zhenzhang","ch10.kaifu",false,true,2},
    {15,"ch10_xiaohuan","trigger_zhenyan","zhenyan","ch10.kaifu","ch10.buzhen"},
    {16,"ch10_xiaohuan","trigger_lifu","lifu","ch10.buzhen","ch10.dingce",false,true,1},
    {17,"ch10_xiaohuan","trigger_zhuji","zhuji","ch10.dingce","ch10.zhuji",false,true,360},
    {18,"ch10_xiaohuan","trigger_dayan","dayan","ch10.zhuji","ch10.huashen",false,true,2295},
    {19,"ch10_xiaohuan","trigger_sanzhuan","sanzhuan","ch10.huashen","ch10.chuguan"},
    {20,"ch10_xiaohuan","trigger_chudao","chudao","ch10.chuguan","ch10.chudao"},
    {21,"ch10_tiandujie","trigger_yunmeng","yunmeng","ch10.chudao","ch10.shuangjiao"},
    {22,"ch10_tiandujie","trigger_baishuilou","baishuilou","ch10.shuangjiao","ch10.liulian"},
    {23,"ch10_tiandujie","trigger_danyaopu","danyaopu","ch10.liulian","ch10.danfang"},
    {24,"ch10_tiandujie","trigger_kezhan","kezhan","ch10.danfang","ch10.jueding",false,true,16},
    {25,"ch10_haichuan","trigger_chuanting","chuanting","ch10.jueding","ch10.chuhai",false,true,33},
    {26,"ch10_jinhai","trigger_huangdao","huangdao","ch10.chuhai","ch10.yingli"},
    {27,"ch10_jinhai","trigger_jiaoshi","jiaoshi","ch10.yingli","ch10.xinshen"},
    {28,"ch10_jinhai","trigger_zhenmen","zhenmen","ch10.bishui_cheng","ch10.taoli"},
    {29,"ch10_jinhai","trigger_ruzhen","ruzhen","ch10.taoli","ch10.zhangu",false,true,75},
    {30,"ch10_haiyuandao","trigger_linshi","linshi","ch10.zhangu","ch10.shadan",false,true,1482},
    {31,"ch10_xiaohuan","trigger_shijin","shijin","ch10.shadan","ch10.shijin"},
    {32,"ch10_xiaohuan","trigger_zhifadui","zhifadui","ch10.shijin","ch10.done",true}
}};

inline std::vector<Hook> hooks() {
    std::vector<Hook> result(kStory.begin(), kStory.end());
    result.push_back({0,"ch10_kuixing","trigger_gujia","gujia","ch10.chudao","ch10.gujia"});
    result.push_back({0,"ch10_jinhai","trigger_bishui_yan","bishui_yan","ch10.xinshen","ch10.bishui_cheng",false,true,1});
    for (const char* direction : {"dong", "nan", "xi", "bei"}) {
        // Static storage keeps Hook's string pointers valid in every TU.
        static const std::map<std::string, std::string> names{
            {"dong","trigger_zhenwei_dong"},{"nan","trigger_zhenwei_nan"},
            {"xi","trigger_zhenwei_xi"},{"bei","trigger_zhenwei_bei"}};
        static const std::map<std::string, std::string> scripts{
            {"dong","zhenwei_dong"},{"nan","zhenwei_nan"},{"xi","zhenwei_xi"},{"bei","zhenwei_bei"}};
        static const std::map<std::string, std::string> seaNames{
            {"dong","trigger_bishui_dong"},{"nan","trigger_bishui_nan"},
            {"xi","trigger_bishui_xi"},{"bei","trigger_bishui_bei"}};
        static const std::map<std::string, std::string> seaScripts{
            {"dong","bishui_dong"},{"nan","bishui_nan"},{"xi","bishui_xi"},{"bei","bishui_bei"}};
        result.push_back({0,"ch10_xiaohuan",names.at(direction).c_str(),scripts.at(direction).c_str(),"ch10.kaifu","",false,false});
        result.push_back({0,"ch10_jinhai",seaNames.at(direction).c_str(),seaScripts.at(direction).c_str(),"ch10.xinshen","",false,false});
    }
    return result;
}

inline std::map<std::string, Json> objects(const Json& map) {
    std::map<std::string, Json> result;
    for (const auto& layer : map.at("layers")) {
        if (layer.value("type", "") != "objectgroup") continue;
        for (const auto& object : layer.at("objects")) {
            const auto name = object.at("name").get<std::string>();
            if (!result.emplace(name, object).second) throw std::runtime_error("Duplicate object " + name);
        }
    }
    return result;
}

inline Json properties(const Json& object) {
    Json result = Json::object();
    for (const auto& property : object.value("properties", Json::array()))
        result[property.at("name").get<std::string>()] = property.at("value");
    return result;
}
} // namespace fanren::test::ch10
