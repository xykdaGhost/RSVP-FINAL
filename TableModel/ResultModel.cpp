#include "ResultModel.h"
#include "QColor"
#include "../ServiceManager/ServiceManager.h"
#include <QDebug>


/**
 * @brief Constructor of ResultModel, set the default value of result
 * @param parent : the parent object
 */
ResultModel::ResultModel(QObject *parent)
    : QAbstractTableModel(parent) {
    //name-number-density, for the first line, max area-min area

    _results.append({"泥渍", 0, 0, 0});
    _results.append({"树叶", 0, 0, 0});
    _results.append({"石头", 0, 0, 0});
    _results.append({"塑料", 0, 0, 0});
    _results.append({"--", 0, 0, 0});
    _results.append({"--", 0, 0, 0});
    _results.append({"--", 0, 0, 0});
    _results.append({"--", 0, 0, 0});
    _results.append({"--", 0, 0, 0});
    _results.append({"总计", 0, 0, 0});
    _results.append({"Level",0, 0, 0});


}

/**
 * @brief Get header data
 * @param section : the section of header
 * @param orientation : the orientation of header, Horizontal or Vertical
 * @param role : data role
 * @return the header data
 */
QVariant ResultModel::headerData(int section, Qt::Orientation orientation, int role) const {
    //return header data by role
    switch(role) {
    case Qt::BackgroundRole:
        if(orientation == Qt::Orientation::Vertical) {
        } else if(section == 0) {
            return QColor(120, 0, 120);
        } else if(section == 1) {
            return QColor(0, 255, 0);
        } else if(section == 2) {
            return QColor(0, 255, 225);
        } else if(section == 3) {
            return QColor(255,0,0);
        } else if(section == 4) {
            return QColor(225, 0, 225);
        } else if(section == 5) {
            return QColor(0, 255, 225);
        } else if(section == 6) {
            return QColor(120, 120, 120);
        } else if(section == 7) {
            return QColor(0, 120, 0);
        } else if(section == 8) {
            return QColor(120, 0, 120);
        }
        break;
    case Qt::TextAlignmentRole:
        //align center for font
        return Qt::AlignCenter;
        break;
    case Qt::DisplayRole:
        //set horizontal header data
        if(orientation == Qt::Orientation::Horizontal) {
            return _results[section][0];
        } else if(section == 0) //set the vertical header data
            return "垃圾数量";
        else if(section == 1)
            return "垃圾密度‰";
        else if (section == 2)
            return "最大面积cm2";


    case Qt::EditRole:
        if(orientation == Qt::Orientation::Horizontal) {
            return _results[section][0];
        } else if(section == 0)
            return "垃圾数量";
        else if(section == 1)
            return "垃圾密度‰";
        else if (section == 2)
            return "最大面积cm2";

    default:
        return QVariant();
    }
}

/**
 * @brief Get row count
 * @param parent : parent index
 * @return the row count
 */
int ResultModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    return _results[0].size() - 1;
}

/**
 * @brief Get column count
 * @param parent : parent index
 * @return the column count
 */
int ResultModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    return _results.size();
}

/**
 * @brief Get data from index
 * @param index : model index
 * @param role : the data role
 * @return the data
 */
QVariant ResultModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();
//    if(index.column() > 0 && index.row() > 2)
//        return QVariant();

    switch(role) {
    case Qt::TextAlignmentRole:
        return Qt::AlignCenter;
    case Qt::DisplayRole:
        return _results[index.column()][index.row() + 1];
    case Qt::EditRole:
        return _results[index.column()][index.row() + 1];
    default:
        return QVariant();
    }
}

void ResultModel::setData(std::vector<std::pair<int, double>> data, std::vector<double> maxBox) {
   int sumNum = 0;
   double sumDig = 0;
   double sumSize = 0;
   for(int i = 0 ; i < data.size(); i++) {
        _results[i][1] = data[i].first;
        _results[i][2] = QString::number(data[i].second * 1000,'f',2).toDouble();
        _results[i][3] = QString::number(maxBox[i],'f',2).toDouble();
        sumNum += data[i].first;
        sumDig += data[i].second * 1000;
        sumSize += maxBox[i];
   }

   _results[9][1] = sumNum;
   _results[9][2] = QString::number(sumDig,'f',2).toDouble();
   _results[9][3] = QString::number(sumSize,'f',2).toDouble();

   _trashLevel = getTrashClass();

   _results[10][1] = _trashLevel;
   _results[10][2] = _trashLevel;
   _results[10][3] = _trashLevel;

   ServiceManager::getInstance().setAmount(_results[9][1].value<int>());
   ServiceManager::getInstance().setDensity(_results[9][2].value<int>());
   ServiceManager::getInstance().setMaxSize(_results[9][3].value<int>());

   int result[12] = {0};
   result[0]    = _results[0][1].value<int>();
   result[2]    = _results[0][2].value<int>() / 10;
   result[1]    = _results[0][3].value<int>();
   result[3]    = _results[1][1].value<int>();
   result[5]    = _results[1][2].value<int>() / 10;
   result[4]    = _results[1][3].value<int>();
   result[6]    = _results[2][1].value<int>();
   result[8]    = _results[2][2].value<int>() / 10;
   result[7]    = _results[2][3].value<int>();
   result[9]    = _results[3][1].value<int>();
   result[11]   = _results[3][2].value<int>() / 10;
   result[10]   = _results[3][3].value<int>();
   ServiceManager::getInstance().setResultArray(result);
   ServiceManager::getInstance().setLevel(_trashLevel);
   emit dataChanged(createIndex(0,0), createIndex(rowCount(), columnCount()));
}

int ResultModel::getTrashClass(){

    if(_results[9][1] >=15 || _results[9][2] >= 50){
        return 3;
    }else if(_results[9][1] >=10 || _results[9][2] >= 30){
        return 2;
    }
    return 1;
}

int ResultModel::getTrash(){

    return _results[9][2].value<int>();
}



