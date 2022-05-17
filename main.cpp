#include <iostream>
#include "httplib.h"
#include <string>
#include <fstream>
#include <stdlib.h>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <mongoc/mongoc.h>

bool sendDocument(mongoc_collection_t *collection, bson_error_t &error, const char* json)
{
  bson_error_t error_bson;
  bson_t      *bson;
  char        *string;

  //const char *json = "{\"name\": {\"first\":\"Grace\", \"last\":\"Hopper\"}}";
  bson = bson_new_from_json ((const uint8_t *)json, -1, &error_bson);

  if (!bson) 
  {
    fprintf (stderr, "%s\n", error_bson.message);
    return false;
  }
  const bool rtnVal = mongoc_collection_insert_one (collection, bson, NULL, NULL, &error);
  string = bson_as_canonical_extended_json (bson, NULL);
  printf ("%s\n", string);
  bson_free (string);
  bson_destroy(bson);
  return rtnVal;
}


struct Registration 
{
  std::string city, state;
  Registration() {}
  Registration(std::string city, std::string state): city(city), state(state) {}
};

class Record
{
  public:
  virtual std::string jsonify() = 0;
  virtual std::string htmlify() = 0;
  virtual void jsonToRecord(std::string json) = 0;
};

class InfoRecord: public Record
{
  public:
  std::string site, type;
    Registration registration;
    InfoRecord() {}
    InfoRecord(std::string site, std::string type, Registration registration): site(site), type(type), registration(registration) {}
    std::string jsonify()
    {
        std::string json;
        json += "{ \"Site\" : \"";
        json += this->site;
        json += "\", \"Type\" : \"";
        json += this->type;
        json += "\", \"Registration\" : {\"City\" : \"";
        json += this->registration.city;
        json += "\", \"State\" : \"";
        json += this->registration.state;
        json += "\"}}";

        return json;
    }

    std::string htmlify()
    {
        std::string html;
        html += "<tr> <td>";
        html += site;
        html += "</td> <td>";
        html += type;
        html += "</td> <td>";
        html += registration.city;
        html += ", ";
        html += registration.state;
        html += "</td> </tr>";

        return html;
    }
    void jsonToRecord(std::string json)
    {
        std::string siteTag = "\"Site\" : \"";
        size_t beginningOfSiteEndex = json.find(siteTag, 0) + siteTag.size();
        std::string siteNTag = "\", \"Type\" : \"";
        size_t endOfSiteIndex = json.find(siteNTag, beginningOfSiteEndex);
        size_t countOfSiteIndex = endOfSiteIndex - beginningOfSiteEndex;
        this->site = json.substr(beginningOfSiteEndex, countOfSiteIndex);

        size_t beginningOfTypeEndex = endOfSiteIndex + siteNTag.size();
        std::string typeNTag = "\", \"Registration\" : { \"City\" : \"";
        size_t endOfTypeIndex = json.find(typeNTag, endOfSiteIndex);
        size_t countOfTypeIndex = endOfTypeIndex - beginningOfTypeEndex;
        this->type = json.substr(beginningOfTypeEndex, countOfTypeIndex);

        size_t beginningOfCityEndex = endOfTypeIndex + typeNTag.size();
        std::string cityNTag = "\", \"State\" : \"";
        size_t endOfCityEndex = json.find(cityNTag, beginningOfCityEndex);
        size_t countOfCityIndex = endOfCityEndex - beginningOfCityEndex;
        this->registration.city = json.substr(beginningOfCityEndex, countOfCityIndex);

        size_t beginningOfStateEndex = endOfCityEndex + cityNTag.size();
        std::string stateNTag = "\" } }";
        size_t endOfStateEndex = json.find(stateNTag, beginningOfStateEndex);
        size_t countOfStateIndex = endOfStateEndex - beginningOfStateEndex;
        this->registration.state = json.substr(beginningOfStateEndex, countOfStateIndex);

        if (std::string::npos == beginningOfSiteEndex)
          throw std::invalid_argument("Error no field named Site found");

        else if (std::string::npos == endOfSiteIndex)
          throw std::invalid_argument("Error no field name Type found");

        else if (std::string::npos == endOfTypeIndex)
          throw std::invalid_argument("Error no field named Registration found");

        else if (std::string::npos == endOfCityEndex)
          throw std::invalid_argument("Error no field named State found");

        else if (std::string::npos == endOfStateEndex)
          throw std::invalid_argument("Error no end of tag for state");
    }



};

struct untrustworthy_sources: public InfoRecord
{
    untrustworthy_sources() {}
    untrustworthy_sources(std::string site, std::string type, Registration registration): InfoRecord(site, type, registration) {}
};

struct trustworthySources: public InfoRecord
{
  trustworthySources() {}
  trustworthySources(std::string site, std::string type, Registration registration): InfoRecord(site, type, registration) {}
};

bool newRecord (mongoc_collection_t *collection, untrustworthy_sources record)
{
  bson_error_t error;
  return sendDocument(collection, error, record.jsonify().c_str());
}

bool newRecord (mongoc_collection_t *collection, trustworthySources record)
{
  bson_error_t error;
  return sendDocument(collection, error, record.jsonify().c_str());
}

std::string queryAndBuildHtml(mongoc_collection_t *collection, bson_t *query)
{
  std::vector<std::string> html;
  html.push_back("<table><tr> <th>Site</th>  <th>Type</th>  <th>Registration</th> </tr>");

  mongoc_cursor_t *cursor;
  const bson_t *doc;
  char *str;
  cursor = mongoc_collection_find_with_opts (collection, query, NULL, NULL);

  while (mongoc_cursor_next (cursor, &doc)) 
  {
    str = bson_as_canonical_extended_json (doc, NULL);
    std::string json(str);
    untrustworthy_sources rec;

    try
    {
      rec.jsonToRecord(json);
      html.push_back(rec.htmlify());
      printf ("%s\n", str);
    }
    catch (std::invalid_argument const& ex)
    {
      std::cout << ex.what() << "\n";
    }

    bson_free (str);
   }
   html.push_back("</table>");
   std::string resultHtml;

   for(const std::string& element : html)
   {
     resultHtml += element;
   }

 
 return resultHtml;

}

std::string queryBasdOnSite(mongoc_collection_t *collection, std::string site)
{
  bson_error_t error_bson;
  bson_t *query = site.size() != 0 ? bson_new_from_json ((const uint8_t *)("{\"Site\":\""+site+"\"}").c_str() , -1, &error_bson) : 
        bson_new_from_json ((const uint8_t *) "{}" , -1, &error_bson);
 std::string result = queryAndBuildHtml(collection, query); 
 bson_destroy(query);
 return result;
}

std::string queryBasdOnType(mongoc_collection_t *collection, std::string type)
{
  bson_error_t error_bson;
  bson_t *query = type.size() != 0 ? bson_new_from_json ((const uint8_t *)("{\"Type\":\""+type+"\"}").c_str() , -1, &error_bson) : 
        bson_new_from_json ((const uint8_t *) "{}" , -1, &error_bson);
 std::string result = queryAndBuildHtml(collection, query); 
 bson_destroy(query);
 return result;
}

//help function prototypes
std::string readFile(std::string filePath);
size_t now();

int main(int argc, char **argv)
{
  //db collection bowler plate

    const char *uri_string = "mongodb+srv://dou18:Salimatou64@cluster0.kkzwi.mongodb.net/myFirstDatabase?retryWrites=true&w=majority";
    mongoc_uri_t *uri;
    mongoc_client_t *client;
    mongoc_database_t *database;
    mongoc_collection_t *untrustworthySources;
    bson_t *command, reply;
    bson_error_t error;
    char *str;
    bool retval;

    /*
    * Required to initialize libmongoc's internals
    */
    mongoc_init ();

    /*
    * Optionally get MongoDB URI from command line
    */
    /*if (argc > 1) {
        //uri_string = argv[1];
    }*/

    /*
    * Safely create a MongoDB URI object from the given string
    */
    uri = mongoc_uri_new_with_error (uri_string, &error);
    if (!uri) {
        fprintf (stderr,
                "failed to parse URI: %s\n"
                "error message:       %s\n",
                uri_string,
                error.message);
        return EXIT_FAILURE;
    }

    /*
    * Create a new client instance
    */
    client = mongoc_client_new_from_uri (uri);
    if (!client) {
        return EXIT_FAILURE;
    }

    /*
    * Register the application name so we can track it in the profile logs
    * on the server. This can also be done from the URI (see other examples).
    */
    mongoc_client_set_appname (client, "connect-example");

    /*
    * Get a handle on the database "db_name" and collection "coll_name"
    */
    database = mongoc_client_get_database (client, "db_name");
    untrustworthySources = mongoc_client_get_collection (client, "db_name", "untrustworthySourcesColl");

    mongoc_collection_t *trustworthySources = mongoc_client_get_collection (client, "db_name", "trustworthySourcesColl");

    //collection = mongoc_client_get_collection (client, "db_name", "coll_untS");

    /*
    * Do work. This example pings the database, prints the result as JSON and
    * performs an insert
    */
    command = BCON_NEW ("ping", BCON_INT32 (1));

    retval = mongoc_client_command_simple (
        client, "admin", command, NULL, &reply, &error);

    if (!retval) {
        fprintf (stderr, "%s\n", error.message);
        return EXIT_FAILURE;
    }

    str = bson_as_json (&reply, NULL);
    printf ("%s\n", str);

    bson_destroy (&reply);
    bson_destroy (command);
    bson_free (str);
    /////////////////////////////////////////////
    // end of db conactor bowler plate
    /////////////////////////////////////////////

  //std::cout<<queryBasdOnSite(collection, "16WMPO.com");

  srand(now());
  httplib::Server svr;
  svr.set_mount_point("/", "./web");
  svr.Get("/option", [untrustworthySources](const auto &req, auto &res) {
    
    std::string responseHTML = "<h2> Search Page by Site</h2>" 
  "<h4><a href=\"/typequery\"> Search by type</a> </h4>"
  "<form action=\"/option\" method=\"GET\">"
  "<label for=\"website\">Search by URL:</label><br>"
  "<input type=\"text\" name=\"website\" value=\"\">"
  "<input type=\"submit\" value=\"Submit\">"
  "</form>" ;
    if (req.has_param("website")) {
        std::string companyName = req.get_param_value("website");
        responseHTML += queryBasdOnSite(untrustworthySources, companyName);
    }
    else {
        responseHTML += queryBasdOnSite(untrustworthySources, "");
    }
    res.set_content(responseHTML, "text/html");
  });

  svr.Get("/typequery", [untrustworthySources](const auto &req, auto &res) {
      
      std::string responseHTML = "<h2> Search Page by Type</h2>" 
      "<h4><a href=\"/option\"> Search by site</a> </h4>"
    "<form action=\"/typequery\" method=\"GET\">"
    "<label for=\"type\">Search by type:</label><br>"
    "<input type=\"text\" name=\"type\" value=\"\">"
    "<input type=\"submit\" value=\"Submit\">"
    "</form>" ;
      if (req.has_param("type")) {
          std::string companyName = req.get_param_value("type");
          responseHTML += queryBasdOnType(untrustworthySources, companyName);
      }
      else {
          responseHTML += queryBasdOnType(untrustworthySources, "");
      }
      res.set_content(responseHTML, "text/html");
    });

  std::cout << "start server..." << std::endl;
  svr.listen("0.0.0.0", 8080);

    /*
    * Release our handles and clean up libmongoc
    */
    mongoc_collection_destroy (untrustworthySources);
    mongoc_collection_destroy (trustworthySources);
    mongoc_database_destroy (database);
    mongoc_uri_destroy (uri);
    mongoc_client_destroy (client);
    mongoc_cleanup ();

  
}

size_t now()
{
  return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
std::string readFile(std::string filePath)
{
  std::fstream file = std::fstream(filePath);
  std::string val((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  return val;
}
