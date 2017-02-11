// HPC_.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <Windows.h>

using boost::asio::ip::tcp;
using json = nlohmann::json;
using namespace std;


class https
{
public:
	https(boost::asio::io_service& io_service, boost::asio::ssl::context& context, boost::asio::ip::tcp::resolver::iterator endpoint_iterator, string domain)
		: socket_(io_service, context)
	{
		ei = endpoint_iterator;
		socket_.set_verify_mode(boost::asio::ssl::context::verify_peer);
		socket_.set_verify_callback(boost::bind(&https::verify_certificate, this, true, _2)); //true => _1
		boost::asio::connect(socket_.lowest_layer(), endpoint_iterator);
		io_service.run();
	}

	bool verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx) {
		char subject_name[256];
		X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
		X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
		//cout << "Verifying:\n" << subject_name << endl;
		return preverified;
	}

	void handle(string method, string domain, string port, string path, string data, string content_type) {
		cout << method + " " + path + " HTTP/1.1 \r\n";
		cout << "Host: " + domain + ":" + port + "\r\n";
		cout << "Accept: */*\r\n";
		cout << "Connection: close\r\n\r\n";

		boost::system::error_code error;
		tcp::resolver::iterator end;
		body = ""; header = "";


		if (end != ei)
			socket_.lowest_layer().connect(*ei++, error);
		else {
			cout << "Error iterator!\n";
		}

		cout << "Connection HTTPS OK!" << endl;
		socket_.handshake(boost::asio::ssl::stream_base::client);

		//Отправляем запрос на сервер
		cout << "Sending request: \n" << endl;
		boost::asio::streambuf request;
		ostream request_(&request);
		
		if (method == "GET") {
			request_ << method + " " + path + " HTTP/1.1 \r\n";
			request_ << "Host: " + domain + ":" + port + "\r\n";
			request_ << "Accept: */*\r\n";
			request_ << "Connection: close\r\n\r\n";
		}
		else if (method == "POST" || method == "PUT" || method == "DELETE") {
			request_ << method + " " + path + " HTTP/1.1 \r\n";
			request_ << "Host: " + domain + ":" + port + "\r\n";
			request_ << "Content-Type: " + content_type + "\r\n"; //application/json
			request_ << "Accept: */*\r\n";
			request_ << "Content-Length: " << data.length() << "\r\n";
			request_ << "Connection: close\r\n\r\n";
			request_ << data;
		}
		

		boost::asio::write(socket_, request);
		boost::asio::streambuf response;
		boost::asio::read_until(socket_, response, "\r\n");

		//Получаем данные с сервера
		istream response_stream(&response);
		string http_version;
		response_stream >> http_version;
		unsigned int status_code;
		response_stream >> status_code;
		string status_message;
		getline(response_stream, status_message);

		if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
			cout << "Invalid response\n";
			return;
		}

		if (status_code != 200) {
			cout << "Response returned with status code " << status_code << "\n";
			return;
		}

		boost::asio::read_until(socket_, response, "\r\n\r\n");

		//Читаем и записываем тело страницы, пришедшей на запрос
		string header;
		while (getline(response_stream, header) && header != "\r")
			this->header += header;
		if (response.size() > 0)
			body += to_string(response);

		while (boost::asio::read(socket_, response, boost::asio::transfer_at_least(1), error))
			body += to_string(response);

		if (error != boost::asio::error::eof)
			throw boost::system::system_error(error);

		socket_.lowest_layer().close();
	}

	string to_string(boost::asio::streambuf &buf)
	{
		ostringstream out;
		out << &buf;
		return out.str();
	}

	string body;
	string header;

private:
	boost::asio::ssl::stream<tcp::socket> socket_;
	boost::asio::ip::tcp::resolver::iterator ei;
	boost::asio::streambuf reply_;
};

string https_connect(string domain, int port, string path_ctx, string method, string path, string data, string content_type) {
	boost::asio::io_service io_service;
	boost::asio::ip::tcp::resolver resolver(io_service);
	boost::asio::ip::tcp::resolver::query query(domain, to_string(port));
	boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
	boost::asio::ssl::context context(boost::asio::ssl::context::sslv23);
	context.add_verify_path(path_ctx);
	https connect(io_service, context, iterator, domain);
	connect.handle(method, domain, to_string(port), path, data, content_type);
	return connect.body;
}

class http
{
public:
	http(boost::asio::io_service& io_service, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
		: socket_(io_service)
	{
		ei = endpoint_iterator;
		boost::asio::connect(socket_.lowest_layer(), ei);
		io_service.run();
	}

	void handle(string method, string domain, string port, string path, string data, string content_type) {
		boost::system::error_code error;
		tcp::resolver::iterator end;
		body = ""; header = "";

		//cout << "Connection HTTP OK!" << endl;
		boost::asio::streambuf request;
		ostream request_(&request);

		if (method == "GET") {
			request_ << method + " " + path + " HTTP/1.1 \r\n";
			request_ << "Host:" + domain + ":" + port + "\r\n";
			request_ << "Accept: */*\r\n";
			request_ << "Connection: close\r\n\r\n";
		}
		else if (method == "POST" || method == "PUT" || method == "DELETE") {
			request_ << method + " " + path + " HTTP/1.1 \r\n";
			request_ << "Host:" + domain + ":" + port + "\r\n";
			request_ << "Content-Type: " + content_type + "\r\n"; //application/json
			request_ << "Accept: */*\r\n";
			request_ << "Content-Length: " << data.length() << "\r\n";
			request_ << "Connection: close\r\n\r\n";
			request_ << data;
		}

		boost::asio::write(socket_, request);
		boost::asio::streambuf response;
		boost::asio::read_until(socket_, response, "\r\n");

		//Получаем данные с сервера
		istream response_stream(&response);
		string http_version;
		response_stream >> http_version;
		unsigned int status_code;
		response_stream >> status_code;
		string status_message;
		getline(response_stream, status_message);

		if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
			cout << "Invalid response\n";
			return;
		}

		cout << "Response returned with status code " << status_code << "\n";

		boost::asio::read_until(socket_, response, "\r\n\r\n");

		//Читаем и записываем тело страницы, пришедшей на запрос
		string header;
		while (getline(response_stream, header) && header != "\r")
			this->header += header;

		if (response.size() > 0)
			body += to_string(response);

		while (boost::asio::read(socket_, response, boost::asio::transfer_at_least(1), error))
			body += to_string(response);

		if (error != boost::asio::error::eof)
			throw boost::system::system_error(error);

		socket_.lowest_layer().close();
	}

	string to_string(boost::asio::streambuf &buf){
		ostringstream out;
		out << &buf;
		return out.str();
	}

	string body;
	string header;

private:
	boost::asio::ip::tcp::resolver::iterator ei;
	boost::asio::streambuf reply_;
	tcp::socket socket_;
};

string http_connect(string domain, int port, string method, string path, string data, string content_type) {
	boost::asio::io_service io_service;
	boost::asio::ip::tcp::resolver resolver(io_service);
	boost::asio::ip::tcp::resolver::query query(domain, to_string(port));
	boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
	http connect(io_service, iterator);
	connect.handle(method, domain, to_string(port), path, data, content_type);
	return connect.body;
}


class network_manager {
public:
	void init() {
		ifstream token_file;
		token_file.open(path_tokens);
		string access_token_temp;
		while (getline(token_file, access_token_temp) && access_token_temp != "")
			token.insert(pair<int, string>(token.size(), access_token_temp));

		//cout << "Counter token = " << counter_token << endl;
		for (int i = 0; i < token.size(); i++)
			cout << token[i] << endl;

		boost::thread* th = new boost::thread(&network_manager::manager, this);
	}

	void add_(string method) {
		//добавляет запрос в очередь
		mutex.lock();
		this->method.push_back(method);
		mutex.unlock();
	}

	void manager() {
		//Управляет потоками. Обрабатывает список запросов.
		//cout << "\n\nStart net_manager manager\n\n" << endl;
		boost::thread_group threads;
		while (true) {
			while (true) { if (!method.empty()) break; }
			while (true) { if (counter_token != token.size()) break; }
			mutex.lock();
			boost::thread* th = new boost::thread(&network_manager::async_connection, this, token[counter_token++]);
			threads.add_thread(th);
			mutex.unlock();
		}
	}

	void async_connection(string at) {
		//cout << "\n\nStart net_manager async = " + at << endl;
		while (true) {
			string method;
			mutex.lock();
			if (this->method.empty()) {
				token[--counter_token] = at;
				//cout << "this->method.empty() = true " << counter_token << endl;
				mutex.unlock();
				return;
			}
			else {
				method = this->method.front();
				this->method.pop_front(); 
				//cout << "this->method.empty() = false " << counter_token << endl;
			}
			mutex.unlock();
			//cout << "\nnetwork_manager::async_connection" + method + "&access_token=" + at << endl;
			save_json("./" + path_parse(method) + ".json", https_connect("api.vk.com", 443, "C:\build\bin\openssl-1.0.2d-x64\bin", "GET", method + "&access_token=" + at, "", ""));
			boost::this_thread::sleep(boost::posix_time::milliseconds(333));
		}
	}

	string path_parse(string path) {
		boost::regex regexp("\/(\.+)\/(\.+\.\.+)\\?(.+\.58)");
		boost::smatch results;
		boost::regex_match(path, results, regexp);
		//cout << "REGEXP = " + results[1] + " " + results[2] + " " + results[3] << endl;
		return results[1] + results[2] + results[3];
	}

private:
	void save_json(string path, string data) {
		//Функция сохранения данных в файл
		ofstream file;
		file.open(path);
		file << data;
		file.close();
	}

	string path_tokens = "./token.txt";
	list<string> method;
	map<int, string> token;
	boost::mutex mutex;
	int counter_token = 0;
} resolver_manager;


class object_manager {
public:
	set<string> groups;

	object_manager(string files) {
		cout << "OM start!!" << endl;
		ifstream file; string line;
		file.open(files);
		while (getline(file, line) && line != "")
			subject_list.push_back(line);
		boost::thread* th = new boost::thread(&object_manager::manager, this);
		group.add_thread(th);
	}

	void manager() {
		cout << "\n\nManager start!!\n\n" << endl;
		while (true) {
			for (auto &item : subject_list)
				init_state(item, false);

			//for (auto &item : file_parse)
				//cout << item << endl;

			while (true) { //true
				mutex.lock();

				//cout << counter_threads;

				if (file_parse.empty()) {
					mutex.unlock();
					cout << "\n\nBreak\n\n";
					break;
				}

				if (counter_threads == max_thread) {
					mutex.unlock();
					continue;
				}

				int temp = max_thread - counter_threads;
				for (int i = 0; i < temp; i++) {
					auto method = file_parse.front(); file_parse.pop_front();

					boost::regex regexp("\/(\.*)\/(\.+\.\.+)\\?(.+\.58)");
					boost::smatch results;
					boost::regex_match(method, results, regexp);
					if (results[2] == "users.get") {
						boost::thread th = boost::thread(&object_manager::parse_about, this, method);
						th.detach();
					}
					else if (results[2] == "groups.get") {
						boost::thread th = boost::thread(&object_manager::parse_group, this, method);
						th.detach();
					}
					else if (results[2] == "video.get") {
						boost::thread th = boost::thread(&object_manager::parse_video, this, method);
						th.detach();
					}
					else if (results[2] == "wall.get") {
						boost::thread th = boost::thread(&object_manager::parse_wall, this, method);
						th.detach();
					}
					else if (results[2] == "wall.getComments") {
						boost::thread th = boost::thread(&object_manager::parse_wall_comments, this, method);
						th.detach();
					}
					else if (results[2] == "video.getComments") {
						boost::thread th = boost::thread(&object_manager::parse_video_comments, this, method);
						th.detach();
					}
					else if (results[2] == "photos.getAll") {
						boost::thread th = boost::thread(&object_manager::parse_photos_get_all, this, method);
						th.detach();
					}
					else if (results[2] == "photos.getComments") {
						boost::thread th = boost::thread(&object_manager::parse_photos_comments, this, method);
						th.detach();
					}
					counter_threads++;
					cout << counter_threads;
					//cout << "\n" << file_parse.size() << endl;
				}
				mutex.unlock();
			}
		}
	}

	void init_state(string id, bool mode) {
		//mode = true => full
		if (id[0] != '-') {
			resolver("groups.get", id, "", 0);
			resolver("users.get", id, "", 0);
		}
		if (mode)
			for (int i = 0; i <= 1000; i += 100)
				resolver("wall.get", id, "", i);
		
			resolver("wall.get", id, "", 0);

		resolver("video.get", id, "", 0);
		resolver("photos.getAll", id, "", 0);
	}

	void resolver(string method, string id, string id_1, int offset) {
		string request;
		if (method == "users.get")
			request = "/method/users.get?user_id=" + id + "&fields=first_name,last_name,about,activities,bdate,books," +
			"career,city,connections,contacts,counters,country,domain,education,home_town,interests," +
			"maiden_name,military,movies,music,nickname,occupation,personal,quotes,relatives,schools,sex,universities,tv&v=" + version;
		else if (method == "groups.get")
			request = "/method/groups.get?user_id=" + id + "&offset=" + to_string(offset) + "&count=1000&v=" + version;
		else if (method == "video.get")
			request = "/method/video.get?owner_id=" + id + "&offset=" + to_string(offset) + "&count=200" + "&v=" + version; //&album_id= 
		else if (method == "wall.get")
			request = "/method/wall.get?owner_id=" + id + "&offset=" + to_string(offset) + "&count=100&v=" + version;
		else if (method == "wall.getComments")
			request = "/method/wall.getComments?owner_id=" + id + "&post_id=" + id_1 +
			"&need_likes=1&preview_length=0&offset=" + to_string(offset) + "&count=100&sort=desc&v=" + version;
		else if (method == "video.getComments")
			request = "/method/video.getComments?owner_id=" + id + "&video_id=" + id_1 +
			"&need_likes=1&preview_length=0&offset=" + to_string(offset) + "&count=100&sort=desc&v=" + version;
		else if (method == "photos.getAll")
			request = "/method/photos.getAll?owner_id=" + id + "&extended=1&offset=" + to_string(offset) + "&count=200&v=" + version;
		else if (method == "photos.getComments")
			request = "/method/photos.getComments?owner_id=" + id + "&photo_id=" + id_1 +
			"&need_likes=1&offset=" + to_string(offset) + "&count=100&sort=desc&v=" + version;

		resolver_manager.add_(request);
		mutex.lock();
		file_parse.push_back(request);
		mutex.unlock();
	}

	/*JSON's file parsing*/

	void parse_about(string method) {}

	void parse_group(string method) {
		cout << method << endl;

		boost::regex regexp("\/method\/groups\.get\?user_id=(\\d+)&offset=(\\d+)&count=1000&v=5\.58");
		boost::smatch results;
		boost::regex_match(method, results, regexp);

		while (!boost::filesystem::exists("./" + path_parse(method) + ".json")) {}
		std::ifstream ifs("./" + path_parse(method) + ".json"); 
		json j; 
		ifs >> j; 
		int iter = 0;

		if (j["response"]["items"].dump() != "null") {
			for (auto const& item : j["response"]["items"]) {
				mutex.lock(); subject_list.push_back("-" + item.dump()); mutex.unlock();
				iter++;
			}

			if (j["response"]["count"].get<int>() != iter && results[2].length() == 1)
				for (int i = 1000; i < j["response"]["count"].get<int>() + 1000; i += 1000)
					resolver("groups.get", results[1], "", i);
		}

		mutex.lock();
		counter_threads--;
		mutex.unlock();

		//cout << method << endl;

		ifs.close();
		boost::filesystem::remove("./" + path_parse(method) + ".json");
	}
	
	void parse_wall(string method) {
		SYSTEMTIME st;
		GetSystemTime(&st);
		cout << method << endl;

		while (!boost::filesystem::exists("./" + path_parse(method) + ".json")) {}

		if (boost::filesystem::is_empty("./" + path_parse(method) + ".json")) {
			boost::this_thread::sleep(boost::posix_time::milliseconds(300));
			//mutex.lock(); counter_threads--; mutex.unlock();
			//return;
		}

		boost::regex regexp("\\/method\\/wall\\.get\\?owner_id=([-]*\\d+)&offset=(\\d+)&count=100&v=5\\.58");
		boost::smatch results; boost::regex_match(method, results, regexp);

		set<string> temp = query_params("wall", results[1]);
		std::ifstream ifs("./" + path_parse(method) + ".json");	json j; ifs >> j;
		ifs.close(); boost::filesystem::remove("./" + path_parse(method) + ".json");

		if (j["response"]["items"].dump() != "null") {
			for (auto const& item : j["response"]["items"]) {
				if (item["text"].dump() != "\"\"" && (temp.find("\"" + item["id"].dump() + "\"") == temp.end() || temp.empty())) {
					test(item["id"].dump(), item["from_id"].dump(), item["text"].dump());
					update_params("wall", results[1], item["id"].dump());
				}
				if (item.find("copy_history") == item.end())
					continue;
				for (auto &temp : item["copy_history"]) {
					if (temp.find("from_id") == temp.end())
						break;
					//resolver("wall.getComments", temp["from_id"].dump(), temp["id"].dump(), 0);
					//resolver("wall.getComments", item["from_id"].dump(), item["id"].dump(), 0);
					break;
				}
			}
		}

		SYSTEMTIME st1; GetSystemTime(&st1);
		cout << "Time: " << st.wMinute << " " << st.wSecond << " " << st.wMilliseconds << " " << st1.wMinute << " " << st1.wSecond << " " << st1.wMilliseconds << "\n\n\n\n" << endl;

		mutex.lock(); counter_threads--; mutex.unlock();
	}

	void parse_wall_comments(string method) {
		boost::regex regexp("\\/method\\/wall\\.getComments\\?owner_id=([-]*\\d+)&post_id=(\\d+)&need_likes=1&preview_length=0&offset=(\\d+)&count=100&sort=desc&v=5\\.58");
		boost::smatch results;
		boost::regex_match(method, results, regexp);

		while (!boost::filesystem::exists("./" + path_parse(method) + ".json")) {}

		if (boost::filesystem::is_empty("./" + path_parse(method) + ".json")) 
			return;

		std::ifstream ifs("./" + path_parse(method) + ".json");
		json j; ifs >> j; int comment = 0;

		if (j["response"]["items"].dump() != "null") {
			for (auto const& item : j["response"]["items"]) {
				test(item["id"].dump(), results[1], item["text"].dump());
				comment++;
			}

			if (j["response"]["count"].get<int>() != comment && results[3].length() == 1)
				for (int i = 100; i < j["response"]["count"].get<int>() + 100; i += 100)
					resolver("wall.getComments", results[1], results[2], i);
					//wall_comments(results[1], results[2], i);
		}
		mutex.lock();
		counter_threads--;
		mutex.unlock();

		//cout << method << endl;

		ifs.close();
		boost::filesystem::remove("./" + path_parse(method) + ".json");
	}

	void parse_video(string method) {
		SYSTEMTIME st;
		GetSystemTime(&st);

		cout << method << endl;

		boost::regex regexp("\\/method\\/video\\.get\\?owner_id=([-]*\\d+)&offset=(\\d+)&count=200&v=5\\.58");
		boost::smatch results;
		boost::regex_match(method, results, regexp);

		while (!boost::filesystem::exists("./" + path_parse(method) + ".json")) {}
		if (boost::filesystem::is_empty("./" + path_parse(method) + ".json")) {
			boost::this_thread::sleep(boost::posix_time::milliseconds(300));
			//mutex.lock(); counter_threads--; mutex.unlock();
			//return;
		}

		std::ifstream ifs("./" + path_parse(method) + ".json");
		json j; ifs >> j; int comment = 0;

		set<string> temp = query_params("video", results[1]);

		if (j["response"]["items"].dump() != "null") {
			for (auto const& item : j["response"]["items"]) {
				if (temp.find("\"" + item["id"].dump() + "\"") == temp.end() || temp.empty()) {
					test(item["id"].dump(), item["owner_id"].dump(), item["title"].dump());
					test(item["id"].dump(), item["owner_id"].dump(), item["description"].dump());
					update_params("video", results[1], item["id"].dump());
					//resolver("video.getComments", item["owner_id"].dump(), item["id"].dump(), 0);
				}
				comment++;
			}

			if (j["response"]["count"].get<int>() != comment && results[2].length() == 1 && temp.empty())
				for (int i = 200; i < j["response"]["count"].get<int>() + 200; i += 200)
					resolver("video.get", results[1], "", i);
		}

		ifs.close();
		boost::filesystem::remove("./" + path_parse(method) + ".json");

		SYSTEMTIME st1;
		GetSystemTime(&st1);

		cout << "Time: " << st.wMinute << " " << st.wSecond << " " << st.wMilliseconds << " " << st1.wMinute << " " << st1.wSecond << " " << st1.wMilliseconds << "\n\n\n\n" << endl;


		mutex.lock();
		counter_threads--;
		mutex.unlock();
	}

	void parse_video_comments(string method) {
		boost::regex regexp("\\/method\\/video\\.getComments\\?owner_id=([-]*\\d+)&video_id=(\\d+)&need_likes=1&preview_length=0&offset=(\\d+)&count=100&sort=desc&v=5\\.58");
		boost::smatch results;
		boost::regex_match(method, results, regexp);

		while (!boost::filesystem::exists("./" + path_parse(method) + ".json")) {}
		if (boost::filesystem::is_empty("./" + path_parse(method) + ".json"))
			return;

		std::ifstream ifs("./" + path_parse(method) + ".json");
		json j; ifs >> j; int comment = 0;

		if (j["response"]["items"].dump() != "null") {
			set<string> video_comment_text;
			for (auto const& item : j["response"]["items"]) {
				test(item["id"].dump(), item["from_id"].dump(), item["text"].dump());
				comment++;
			}

			if (j["response"]["count"].get<int>() != comment && results[3].length() == 1)
				for (int i = 100; i < j["response"]["count"].get<int>() + 100; i += 100)
					resolver("video.getComments", results[1], results[2], i);
					//video_comments(results[1], results[2], i);
		}
		mutex.lock();
		counter_threads--;
		mutex.unlock();

		//cout << method << endl;

		ifs.close();
		boost::filesystem::remove("./" + path_parse(method) + ".json");
	}

	void parse_photos_get_all(string method) {
		SYSTEMTIME st;
		GetSystemTime(&st);
		cout << method << endl;

		boost::regex regexp("\\/method\\/photos\\.getAll\\?owner_id=([-]*\\d+)&extended=1&offset=(\\d+)&count=200&v=5\\.58");
		boost::smatch results;
		boost::regex_match(method, results, regexp);

		while (!boost::filesystem::exists("./" + path_parse(method) + ".json")) {}
		if (boost::filesystem::is_empty("./" + path_parse(method) + ".json")) {
			boost::this_thread::sleep(boost::posix_time::milliseconds(300));
			//mutex.lock(); counter_threads--; mutex.unlock();
			//return;
		}

		std::ifstream ifs("./" + path_parse(method) + ".json");
		json j; ifs >> j; int comment = 0;

		set<string> temp = query_params("photo", results[1]);

		if (j["response"]["items"].dump() != "null") {
			for (auto const& item : j["response"]["items"]) {
				if (temp.find("\"" + item["id"].dump() + "\"") == temp.end() || temp.empty()) {
					test(item["id"].dump(), results[1], item["text"].dump());
					update_params("photo", results[1], item["id"].dump());
					//resolver("photos.getComments", results[1], item["id"].dump(), 0);
				}
				comment++;
			}

			if (j["response"]["count"].get<int>() != comment && results[2].length() == 1 && temp.empty())
				for (int i = 200; i < j["response"]["count"].get<int>() + 200; i += 200)
					resolver("photos.getAll", results[1], "", i);
		}

		ifs.close();
		boost::filesystem::remove("./" + path_parse(method) + ".json");

		SYSTEMTIME st1;
		GetSystemTime(&st1);

		cout << "Time: " << st.wMinute << " " << st.wSecond << " " << st.wMilliseconds << " " << st1.wMinute << " " << st1.wSecond << " " << st1.wMilliseconds << "\n\n\n\n" << endl;

		mutex.lock();
		counter_threads--;
		mutex.unlock();
	}

	void parse_photos_comments(string method) {
		boost::regex regexp("\\/method\\/photos\\.getComments\\?owner_id=([-]*\\d+)&photo_id=(\\d+)&need_likes=1&offset=(\\d+)&count=100&sort=desc&v=5\\.58");
		boost::smatch results;
		boost::regex_match(method, results, regexp);

		while (!boost::filesystem::exists("./" + path_parse(method) + ".json")) {}
		if (boost::filesystem::is_empty("./" + path_parse(method) + ".json")) 
			return;

		std::ifstream ifs("./" + path_parse(method) + ".json");
		json j; ifs >> j; int comment = 0;

		if (j["response"]["items"].dump() != "null") {
			for (auto const& item : j["response"]["items"]) {
				test(item["id"].dump(), results[1], item["text"].dump());
				comment++;
			}

			if (j["response"]["count"].get<int>() != comment && results[3].length() == 1)
				for (int i = 100; i < j["response"]["count"].get<int>() + 100; i += 100)
					resolver("photos.getComments", results[1], results[2], i);
					//photos_comments(results[1], results[2], i);
		}

		mutex.lock();
		counter_threads--;
		mutex.unlock();

		//cout << method << endl;

		ifs.close();
		boost::filesystem::remove("./" + method + ".json");
	}


	/*ElasticSearch*/
	set<string> query_params(string type_params, string id_user) {
		set<string> results;
		//cout << "GET localhost:9200/subject/node/" + id_user + "?_source=" + type_params + "&pretty" << endl;
		//cout << "\n\n\nGET: " << conn.header_response() << "   " << conn.text_response() << "   /subject/node/" + id_user + "?_source=" + type_params + "&pretty" << endl;

		ofstream out("temp" + type_params + "__" + id_user + ".txt");
		out << http_connect("localhost", 9200, "GET", "/subject/node/" + id_user + "?_source=" + type_params + "&pretty", "", "");
		out.close();
		ifstream in("temp" + type_params + "__" + id_user + ".txt");
		json j;
		j << in;
		in.close();
		boost::filesystem::remove("temp" + type_params + "__" + id_user + ".txt");

		if (j["found"].dump() == "true")
			for (auto &i : j["_source"][type_params])
				results.insert(i.dump());
		else if (j["found"].dump() == "false")
			create_doc(id_user);

		return results;
	}

	void create_doc(string id_user) {
		string data = "{\"id\": \"" + id_user + "\", \"wall\": [], \"photo\": [],\"video\": []}";
		http_connect("localhost", 9200, "POST", "/subject/node/" + id_user, data, "application/json");

		//cout << "POST localhost:9200/subject/node/" + id_user<< data << endl;
		//cout << "Create document POST: " << connection.header() << "   " << connection.response() << endl;
	}

	void update_params(string type_params, string id_user, string new_id) {
		string data = "{\"script\" : {\"inline\": \"ctx._source." + type_params + ".add(params." + type_params
			+ ")\", \"lang\" : \"painless\",\"params\" : {\"" + type_params + "\" : \"" + new_id + "\"}}}";
		http_connect("localhost", 9200, "POST", "/subject/node/" + id_user, data, "application/json");

		//cout << "POST localhost:9200/subject/node/" + id_user + "/_update " << data << endl;
		//cout << "POST: " << conn.header_response() << "   " << conn.text_response() << endl;
	}

	void test(string id, string ids, string text) {
		if (text == "\"\"" || text == "")
			return;
		string term = "смерть смерти умери сдохни белый черный русь русич биться бить российский мудак русский мразь власть слава руси россии гад чурка чуркам убирайся вставай собирайся вонь русорвань русичи русичей русичам американцы американцам хахлы хохлов хохла хохлу евреи евреем евреям еврею рабы раб рабам рабский рабские алкоголь курение эмигрант эмиграция аборт коррупция религия религиозные вера верующие расизм расист национализм националист политика политик правительство власть инвалид союз славянин родина гей зоофил голубой чурка враг еврей жид джихад убийство резать";
		float counter = 0; long put;

		text = cp1251_to_utf8(&utf8_to_cp1251(text)[0u]); 
		while (put = text.find("\"") && put != string::npos)
			text.erase(put, 1);
 
		http_connect("localhost", 9200, "POST", "/extrimism/data/" + id + ids, "{\"ids\": \"" + ids + "\", \"text\": " + text + "}", "application/json");

		//cout << "POST: " << connection.header() << "   " << connection.response() << "text = " + utf8_to_cp1251(text) << endl;

		string found = "{ \"query\":{ \"more_like_this\" : { \"fields\" : [\"text\"], \"like\" : \"" + cp1251_to_utf8(&term[0u]) +
			"\", \"min_term_freq\" : 1, \"min_doc_freq\": 1, \"max_query_terms\" : 10}}}";
		
		//cout << "PUT: " << connection1.header() << "   " << utf8_to_cp1251(connection1.response()) << endl;
		/* std::string text = R"("+http_connect("localhost", 9200, "POST", "/extrimism/_search?q=ids:\"" + ids + "\"", found, "application/json")+R")";
		json j = json::parse(text);*/

		ofstream out("temp" + id + ids + ".txt");	
		out << http_connect("localhost", 9200, "POST", "/extrimism/_search?q=ids:\"" + ids + "\"", found, "application/json");
		out.close();
		ifstream in("temp" + id + ids + ".txt");	
		json j;
		j << in;

		for (auto &i : j["hits"]["hits"])
			if (i["_id"] == (id + ids))
				counter += i["_score"].get<float>();
		in.close();
		boost::filesystem::remove("temp" + id + ids + ".txt");


		if (counter == 0) {
			auto response = http_connect("localhost", 9200, "DELETE", "/extrimism/data/" + id + ids, "{\"ids\": \"" + ids + "\", \"text\": " + text + "}", "application/json");
			cout << "DELETE: " << utf8_to_cp1251(response) << endl;
		}
		else {
			black_list.insert(pair<string, float>(ids, counter + black_list[id]));
		}
	}


	void create_node() {
		/*Создает пустой объект узла*/
	}

	void update_node() {
		/*Обновляет данные узла*/
	}

	void delete_node() {
		/*Удаляет узел*/
	}

	bool query_options() {
		
	}

	void processing_data() {
		/*Обработка данных. 
		Поиск схожих запросов поисковике. 
		Категоризация текста и дальнешое тегирование.*/
	}


	string utf8_to_cp1251(std::string const & utf8)
	{
		if (!utf8.empty())
		{
			int wchlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.size(), NULL, 0);
			if (wchlen > 0 && wchlen != 0xFFFD)
			{
				vector<wchar_t> wbuf(wchlen);
				MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.size(), &wbuf[0], wchlen);
				vector<char> buf(wchlen);
				WideCharToMultiByte(1251, 0, &wbuf[0], wchlen, &buf[0], wchlen, 0, 0);

				return string(&buf[0], wchlen);
			}
		}
		return string();
	}

	string cp1251_to_utf8(const char *str)
	{
		string res;
		int result_u, result_c;
		result_u = MultiByteToWideChar(1251, 0, str, -1, 0, 0);
		if (!result_u)
			return 0;

		wchar_t *ures = new wchar_t[result_u];

		if (!MultiByteToWideChar(1251, 0, str, -1, ures, result_u)){
			delete[] ures;
			return 0;
		}

		result_c = WideCharToMultiByte(CP_UTF8, 0, ures, -1, 0, 0, 0, 0);
		if (!result_c){
			delete[] ures;
			return 0;
		}

		char *cres = new char[result_c];

		if (!WideCharToMultiByte(CP_UTF8, 0, ures, -1, cres, result_c, 0, 0)){
			delete[] cres;
			return 0;
		}
		delete[] ures;
		res.append(cres);
		delete[] cres;
		return res;
	}

	string path_parse(string path) {
		boost::regex regexp("\/(\.+)\/(\.+)\\?(\.+)");
		boost::smatch results;
		boost::regex_match(path, results, regexp);
		//cout << "REGEXP = " + results[1] + " " + results[2] + " " + results[3] << endl;
		return results[1] + results[2] + results[3];
	}


private:
	string version = "5.58", id;
	int counter_threads = 0, max_thread = 4;
	list<string> subject_list, file_parse;
	map<string, int> black_list;
	boost::mutex mutex;
	boost::thread_group group;
};




int main() {
	//https://oauth.vk.com/authorize?client_id=5192360&display=page&redirect_uri=https://oauth.vk.com/blank.html&scope=notify,friends,photos,audio,video,pages,status,notes,messages,wall,ads,offline,docs,groups,notifications,stats,email&response_type=token&v=5.56
	setlocale(LC_ALL, "Russian");
	//Сначала обрабатываются пользователи, по ктором собирается списко групп. 
	//Далее борабатываются полученные группы.
	resolver_manager.init();
	object_manager om("./users.txt");
	cin.get();
	return 0;
}
