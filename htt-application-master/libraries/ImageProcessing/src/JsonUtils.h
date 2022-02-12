//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __JSONUTILS_H__
#define __JSONUTILS_H__

#include "json/json.h"
// #include <initializer_list>

class JsonUtils
{
private:
    class JsonValue : public Json::Value
    {
    public:
        template <typename T> JsonValue(T value) : Json::Value(value)
        {
        }
        JsonValue(std::initializer_list<std::pair<std::string, JsonValue>> values)
        {
            for (auto it = values.begin(); it != values.end(); it++)
            {
                std::pair<std::string, JsonValue> attr = *it;
                Json::Value& v = *this;
                v[attr.first] = attr.second;
            }
        }
    };

public:
    /**
     * Quickly create a JSON value, using the initialization list.
     * Example:
     * 		json({
     * 			{"field1":"stringvalue"},
     * 			{"field2":{
     * 				{"nested":1}
     * 			}
     * 		});
     */
    static Json::Value json(std::initializer_list<std::pair<std::string, JsonValue>> value)
    {
        return JsonValue(value);
    }
    static Json::Value json(std::string stringJson)
    {
        std::stringstream sstr(stringJson);
        Json::Value json;
        sstr >> json;
        return json;
    }
};

#endif //__JSONUTILS_H__
