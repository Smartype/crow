#pragma once
#include <fstream>
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include "http_request.h"
#include "http_response.h"

namespace crow
{
    using namespace boost::filesystem;

    struct DocumentMiddleWare
    {
        std::string document_root;

        struct context
        {
        };

        DocumentMiddleWare() : document_root{"."}
        {
        }

        void setDocumentRoot(const std::string& doc) 
        {
            document_root = doc;
        }

        void before_handle(request& req, response& res, context& ctx)
        {
        }

        void after_handle(request& req, response& res, context& ctx)
        {
            if (res.code != 404)
                return;

            // TODO: check if file outside of document_root
            path filename{document_root};
            // ignore url params
            filename /= req.url;

            CROW_LOG_DEBUG << "filename: " << filename.string();

            file_status s = status(filename);
            if (is_directory(s))
            {
                if (req.raw_url.back() != '/') 
                {
                    CROW_LOG_DEBUG << "directory without tail /, redirect with 301";
                    res.code = 301;
                    res.body.clear();
                    res.set_header("Location", req.raw_url + '/');
                    return;
                }

                filename /= "index.html";
                s = status(filename);
            }

            if (!is_regular_file(s))
            {
                CROW_LOG_DEBUG << "not a regular file";
                return;
            }

            const std::string spath{filename.string()};
            std::ifstream inf{std::cref(spath)};
            if (!inf)
            {
                CROW_LOG_DEBUG << "failed to read file";
                res.code = 400;
                return;
            }

            std::string body{std::istreambuf_iterator<char>(inf), std::istreambuf_iterator<char>()};
            res.code = 200;
            res.body = body; 
            res.set_header("Cache-Control", "public, max-age=86400");
            if (filename.has_extension())
            {
                std::string ext = filename.extension().string();
                boost::algorithm::to_lower(ext);
                static std::unordered_map<std::string, std::string> mimeTypes = {
                    { "html", "text/html; charset=utf-8" },
                    { "htm", "text/html; charset=utf-8" },
                    { "js", "application/javascript; charset=utf-8" },
                    { "css", "text/css; charset=utf-8" },
                    { "gif", "image/gif" },
                    { "png", "image/png" },
                };

                const auto& mime = mimeTypes.find(ext);
                if (mime != mimeTypes.end())
                {
                    res.set_header("Content-Type", mime->second);
                }
            }
        }
    };
}
