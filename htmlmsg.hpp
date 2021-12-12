#pragma once
#include <iostream>
#include <string>
#include <boost/algorithm/string/replace.hpp>

void escape(std::string& content) {
    boost::replace_all(content, "&", "&amp;");
    boost::replace_all(content, "\r", "");
    boost::replace_all(content, "\n", "&NewLine;");
    boost::replace_all(content, "<", "&lt;");
    boost::replace_all(content, ">", "&gt;");
    boost::replace_all(content, "\'", "&#39;");
    boost::replace_all(content, "\"", "&#34;");
}

void printShell(int session, std::string content) {
    escape(content);
    std::cout << "<script>document.getElementById('s" + std::to_string(session) + "').innerHTML += '" + content + "';</script>\n";
    std::cout.flush();
}

void printCMD(int session, std::string content) {
    escape(content);
    std::cout << "<script>document.getElementById('s" + std::to_string(session) + "').innerHTML += '<b>" + content + "</b>';</script>\n";
    std::cout.flush();
}

void printTableTitle(int session, std::string host, std::string port) {
    std::string url = host + ":" + port;
    std::cout << "<script>document.getElementById('s" + std::to_string(session)  + "title').innerHTML += '"+ url  +"';</script>\n";
    std::cout.flush();
}

void printHeader() {
    std::cout << "Content-type: text/html\r\n\r\n";
}

void printHtmlHead() {
    std::cout << "<!DOCTYPE html>\n"
              << "<html lang=\"en\">\n";
}

void printBodyHead() {
    std::cout <<    "<head>\n"
              <<        "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n"
              <<        "<title>NP Project 3 Sample Console</title>\n"
              <<        "<link\n"
              <<        "rel=\"stylesheet\"\n"
              <<        "href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"\n"
              <<        "integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\n"
              <<        "crossorigin=\"anonymous\"\n"
              <<        "/>\n"
              <<        "<link\n"
              <<        "href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n"
              <<        "rel=\"stylesheet\"\n"
              <<        "/>\n"
              <<        "<link\n"
              <<        "rel=\"icon\"\n"
              <<        "type=\"image/png\"\n"
              <<        "href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\n"
              <<        "/>\n"
              <<        "<style>\n"
              <<        "* {\n"
              <<        "    font-family: 'Source Code Pro', monospace;\n"
              <<        "    font-size: 1rem !important;\n"
              <<        "}\n"
              <<        "body {\n"
              <<        "    background-color: #212529;\n"
              <<        "}\n"
              <<        "pre {\n"
              <<        "    color: #cccccc;\n"
              <<        "}\n"
              <<        "b {\n"
              <<        "    color: #01b468;\n"
              <<        "}\n"
              <<        "</style>\n"
              <<    "</head>\n";
}

void printBody() {
    std::cout <<    "<body>\n"
              <<        "<table class=\"table table-dark table-bordered\">\n"
              <<        "<thead>\n"
              <<            "<tr>\n"
              <<            "<th id=\"s0title\"scope=\"col\"></th>\n"
              <<            "<th id=\"s1title\"scope=\"col\"></th>\n"
              <<            "<th id=\"s2title\"scope=\"col\"></th>\n"
              <<            "</tr>\n"
              <<        "</thead>\n"
              <<        "<tbody>\n"
              <<            "<tr>\n"
              <<            "<td><pre id=\"s0\" class=\"mb-0\"></pre></td>\n"
              <<            "<td><pre id=\"s1\" class=\"mb-0\"></pre></td>\n"
              <<            "<td><pre id=\"s2\" class=\"mb-0\"></pre></td>\n"
              <<            "</tr>\n"
              <<        "</tbody>\n"
              <<        "</table>\n"
              <<        "<table class=\"table table-dark table-bordered\">\n"
              <<        "<thead>\n"
              <<            "<tr>\n"
              <<            "<th id=\"s3title\"scope=\"col\"></th>\n"
              <<            "<th id=\"s4title\"scope=\"col\"></th>\n"
              <<            "</tr>\n"
              <<        "</thead>\n"
              <<        "<tbody>\n"
              <<            "<tr>\n"
              <<            "<td><pre id=\"s3\" class=\"mb-0\"></pre></td>\n"
              <<            "<td><pre id=\"s4\" class=\"mb-0\"></pre></td>\n"
              <<            "</tr>\n"
              <<        "</tbody>\n"
              <<        "</table>\n"
              <<    "</body>\n";
}

void printHtmlEnd() {
    std::cout <<"</html>\n";
}

void printHtmltemplate() {
    printHeader();
    printHtmlHead();
    printBodyHead();
    printBody();
    printHtmlEnd();
    std::cout.flush();
}
