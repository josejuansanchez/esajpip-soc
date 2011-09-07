#define SHOW_TRACES
#include "trace.h"
#include "request.h"


namespace http
{

  void Request::ParseParameter(istream& stream, const string& param, string& value)
  {
    getline(stream, value, '&');
  }

  void Request::ParseParameters(istream& stream)
  {
    string param, value;

    parameters.clear();

    while(stream.good()) {
      value.clear();
      getline(stream, param, '=');
      ParseParameter(stream, param, value);
      if(stream) parameters[param] = value;
    }
  }

  istream& operator >> (istream &in, Request &request)
  {
    string line, cad, uri;
    request.type = Request::UNKNOWN;

    if(getline(in, line)) {
      if(line.size() <= 0) in.setstate(istream::failbit);
      else {
        TRACE("HTTP Request: " << line);

        istringstream in_str(line);

        if(!(in_str >> cad >> uri >> request.protocol)) in.setstate(istream::failbit);
        else {
          if(cad == "POST") request.type = Request::POST;
          else if(cad == "GET") request.type = Request::GET;
          else in.setstate(istream::failbit);
          request.ParseURI(uri);
        }
      }
    }

    return in;
  }

  ostream& operator << (ostream& out, const Request& request)
  {
    if(request.type != Request::UNKNOWN) {
      out << (request.type == Request::GET ? "GET" : "POST") << " " << request.object;

      if(request.parameters.size() > 0) {
        out << "?";
        map<string, string>::const_iterator i = request.parameters.begin();

        if(!i->second.empty()) out << i->first << "=" << i->second;
        else out << i->first;

        while(++i != request.parameters.end()) {
          out << "&";
          if(!i->second.empty()) out << i->first << "=" << i->second;
          else out << i->first;
        }
      }

      out << " " << request.protocol << Protocol::CRLF;
    }

    return out;
  }

}
