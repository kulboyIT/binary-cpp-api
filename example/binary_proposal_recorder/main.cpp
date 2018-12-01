#include "BinaryAPI.hpp"
#include "xtime.hpp"
#include <fstream>
#include <dir.h>
#include <stdlib.h>

#define BUILD_VER 1.1

using json = nlohmann::json;

std::string format(const char *fmt, ...);
std::vector<std::string> get_all_symbols_binary();
void make_commit(std::string disk, std::string path, std::string new_file);

int main() {
        std::cout << "build version " << (float)BUILD_VER << std::endl;
        BinaryApi iBinaryApi;
        BinaryApi iBinaryApiForTime;
        json j_settings;
        std::ifstream i("settings.json");
        i >> j_settings;
        i.close();
        std::cout << std::setw(4) << j_settings << std::endl;
        std::vector<std::string> symbols = get_all_symbols_binary();
        // инициализируем список валютных пар
        std::cout << "init_symbols..." << std::endl;
        iBinaryApi.init_symbols(symbols);
        // инициализируем поток процентов выплат
        const double amount = j_settings["amount"];
        const int duration = j_settings["duration"];
        const int duration_uint = j_settings["duration_uint"];
        const std::string currency = j_settings["currency"];
        const std::string folder_name = j_settings["folder"];
        const std::string disk_name = j_settings["disk"];
        const std::string path = j_settings["path"];
        int is_use_git = j_settings["git"];
        std::string old_file_name = "";

        // сохраним список валютных пар и параметры в отдельный файл
        json j_pp;
        j_pp["amount"] = amount;
        j_pp["duration"] = duration;
        j_pp["duration_uint"] = duration_uint;
        j_pp["currency"] = currency;
        j_pp["symbols"] = symbols;

        std::string folder_path = disk_name + ":\\" + path + "\\" + folder_name;
        std::string file_name_pp = folder_path + "\\parameters.json";
        mkdir(folder_path.c_str());
        std::ofstream fp(file_name_pp);
        fp << std::setw(4) << j_pp << std::endl;
        fp.close();
        //

        std::cout << "..." << std::endl;
        unsigned long long servertime_last = 0;
        while(true) {
                // для всех валютных пар
                std::vector<double> buy_data; // проценты выплат
                std::vector<double> sell_data;
                unsigned long long servertime = 0;

                if(!iBinaryApi.is_proposal_stream()) {
                        // инициализируем поток процентов выплат
                        std::cout << "init_stream_proposal..." << std::endl;
                        int err_data = iBinaryApi.init_stream_proposal(amount, duration, duration_uint, currency);
                        std::cout << "err: " << err_data << std::endl;
                        if(err_data != iBinaryApi.OK)
                                continue;
                }

                if(iBinaryApiForTime.get_servertime(servertime) != iBinaryApiForTime.OK) {
                        iBinaryApiForTime.request_servertime();
                        std::this_thread::sleep_for(std::chrono::milliseconds(450));
                        continue;
                } else {
                        if(servertime > servertime_last) {
                                servertime_last = servertime;
                        } else {
                                std::this_thread::yield();
                                continue;
                        }
                }
#               if(1)
                if(iBinaryApi.get_stream_proposal(buy_data, sell_data) == iBinaryApi.OK) {
                        // выводим последние значения котировок валютных пар
                        for(size_t i = 0; i < symbols.size(); ++i) {
                                std::cout << symbols[i] << " " << buy_data[i] << "/" << sell_data[i] << std::endl;
                        }
                        // составляем список
                        json j;
                        j["time"] = servertime;
                        for(size_t i = 0; i < symbols.size(); ++i) {
                                j["data"][i]["symbol"] = symbols[i];
                                j["data"][i]["buy"] = format("%.3f",buy_data[i]);
                                j["data"][i]["sell"] = format("%.3",sell_data[i]);
                        }
                        // сохраняем список
                        mkdir(folder_path.c_str());
                        xtime::DateTime iTime(servertime);
                        std::string file_chunk_name = "proposal_" +
                                                      std::to_string(iTime.day) + "_" +
                                                      std::to_string(iTime.month) + "_" +
                                                      std::to_string(iTime.year);

                        const std::string file_name = folder_path + "\\" +
                                                      file_chunk_name +
                                                      ".json";
                        // если не выходной, сохраняем файлы
                        if(!xtime::is_day_off(servertime)) {
                                std::ofstream o(file_name, std::ios::app);
                                o << j.dump() << std::endl;
                                o.close();
                        }

                        if(old_file_name != file_name && is_use_git) {
                                std::thread make_thread([=]() {
                                        if(old_file_name == "") {
                                                make_commit(disk_name, path, file_name);
                                        } else {
                                                make_commit(disk_name, path, old_file_name);
                                        }
                                });
                                make_thread.detach();
                                old_file_name = file_name;
                        }
                }
#               endif
                // время сервера в GMT\UTC
                std::cout << servertime << std::endl;
                std::cout << std::endl;
                std::this_thread::yield();
        }
}

std::vector<std::string> get_all_symbols_binary() {
        std::vector<std::string> vsymbol;
        vsymbol.push_back("WLDAUD");
        vsymbol.push_back("WLDEUR");
        vsymbol.push_back("WLDGBP");
        vsymbol.push_back("WLDUSD");

        vsymbol.push_back("frxAUDCAD");
        vsymbol.push_back("frxAUDCHF");
        vsymbol.push_back("frxAUDJPY");
        vsymbol.push_back("frxAUDNZD");
        vsymbol.push_back("frxAUDUSD");

        vsymbol.push_back("frxEURAUD");
        vsymbol.push_back("frxEURCAD");
        vsymbol.push_back("frxEURCHF");
        vsymbol.push_back("frxEURGBP");
        vsymbol.push_back("frxEURJPY");
        vsymbol.push_back("frxEURNZD");
        vsymbol.push_back("frxEURUSD");

        vsymbol.push_back("frxGBPAUD");
        vsymbol.push_back("frxGBPCAD");
        vsymbol.push_back("frxGBPCHF");
        vsymbol.push_back("frxGBPJPY");

        //vsymbol.push_back("frxGBPNOK");
        vsymbol.push_back("frxGBPNZD");
        //vsymbol.push_back("frxGBPPLN");
        //vsymbol.push_back("frxGBPUSD");

        //vsymbol.push_back("frxNZDJPY");
        vsymbol.push_back("frxNZDUSD");

        vsymbol.push_back("frxUSDCAD");
        //vsymbol.push_back("frxUSDCHF");
        vsymbol.push_back("frxUSDJPY");
        //vsymbol.push_back("frxUSDNOK");
        //vsymbol.push_back("frxUSDPLN");
        //vsymbol.push_back("frxUSDSEK");
        return vsymbol;
}

void make_commit(std::string disk, std::string path, std::string new_file)
{
        std::string str_return_hd = "cd " + disk + ":\\";
        std::cout << str_return_hd << std::endl;
        //system(str_return_hd.c_str());

        std::string str_cd_path = "cd " + path;
        std::cout << str_cd_path << std::endl;
        //system(str_cd_path.c_str());

        std::string str_git_add = "git add " + new_file;
        std::cout << str_git_add << std::endl;
        //system(str_git_add.c_str());

        std::string str_git_commit = "git commit -a -m \"update\"";
        std::cout << str_git_commit << std::endl;
        //system(str_git_commit.c_str());

        std::string str_git_pull = "git pull";
        std::cout << str_git_pull << std::endl;
        //system(str_git_pull.c_str());

        std::string str_git_push = "git push";
        std::cout << str_git_push << std::endl;
        //system(str_git_push.c_str());

        std::string msg = str_return_hd + " && " + str_cd_path + " && " + str_git_add + " && " + str_git_commit + " && " + str_git_pull + " & " + str_git_push;
        system(msg.c_str());
}

std::string format(const char *fmt, ...)
{
        va_list args;
        va_start(args, fmt);
        std::vector<char> v(1024);
        while (true)
        {
                va_list args2;
                va_copy(args2, args);
                int res = vsnprintf(v.data(), v.size(), fmt, args2);
                if ((res >= 0) && (res < static_cast<int>(v.size())))
                {
                    va_end(args);
                    va_end(args2);
                    return std::string(v.data());
                }
                size_t size;
                if (res < 0)
                    size = v.size() * 2;
                else
                    size = static_cast<size_t>(res) + 1;
                v.clear();
                v.resize(size);
                va_end(args2);
        }
}
