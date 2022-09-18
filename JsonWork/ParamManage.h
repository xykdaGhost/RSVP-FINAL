#ifndef PARAMMANAGE_H
#define PARAMMANAGE_H
#include <jsoncpp/json/json.h>
#include "JsonWork.h"
#include "../TreeModel/ParameterModel.h"
/**
 * @brief The ParamManage class
 * @note save the parameters from json file
 */
class ParamManage : public JsonWork {
public:
    static ParamManage& getInstance() {
        static ParamManage paramManage("/home/mv/release/settings.json");
        return paramManage;
    }
    ~ParamManage() override { };

    void parseJsonFromFile(std::string filename) override;
    void parseJsonFromString(std::string str) override;
    void writeJsonToFile(std::string filename) override;
    void updateJsonRoot() override;
    void updateJsonStruct() override;
    std::string getJsonString() override;
    ParameterModel* model() const { return _parameterModel; }

private:
    explicit ParamManage(const std::string& filename);

    ParameterModel* _parameterModel;

};

#endif // PARAMMANAGE_H
