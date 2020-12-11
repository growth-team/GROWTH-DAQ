/*
 * FitsFile.hh
 *
 *  Created on: Jun 18, 2014
 *      Author: yuasa
 */

#ifndef CXXUTILITIES_FITSFILE_HH_
#define CXXUTILITIES_FITSFILE_HH_

#include "CxxUtilities/String.hh"

namespace CxxUtilities {

class FitsUtility {
#ifndef TBIT
public:
	// DATATYPE               TFORM CODE
	static const int TBIT = 1; //                            'X'
	static const int TBYTE = 11; // 8-bit unsigned byte,       'B'
	static const int TLOGICAL = 14; // logicals (int for keywords and char for table cols   'L'
	static const int TSTRING = 16; // ASCII string,              'A'
	static const int TSHORT = 21; // signed short,              'I'
	static const int TLONG = 41; // signed long,
	static const int TLONGLONG = 81; // 64-bit long signed integer 'K'
	static const int TFLOAT = 42; // single precision float,    'E'
	static const int TDOUBLE = 82; // double precision float,    'D'
	static const int TCOMPLEX = 83; // complex (pair of floats)   'C'
	static const int TDBLCOMPLEX = 163; // double complex (2 doubles) 'M'

	static const int TINT = 31; // int
	static const int TSBYTE = 12; // 8-bit signed byte,         'S'
	static const int TUINT = 30; // unsigned int               'V'
	static const int TUSHORT = 20; // unsigned short             'U'
	static const int TULONG = 40; // unsigned long
#endif

public:
	static std::string convertSIB2StyleDataTypeNameToCfitsioStyleTFORM(std::string dataTypeName) {
		dataTypeName = CxxUtilities::String::downCase(dataTypeName);
		if (CxxUtilities::String::include(dataTypeName, "byte")) {
			if (CxxUtilities::String::include(dataTypeName, "unsigned")) {
				return "B";
			} else {
				return "S";
			}
		} else if (CxxUtilities::String::include(dataTypeName, "short")) {
			if (CxxUtilities::String::include(dataTypeName, "unsigned")) {
				return "U";
			} else {
				return "I";
			}
		} else if (CxxUtilities::String::include(dataTypeName, "int")) {
			if (CxxUtilities::String::include(dataTypeName, "unsigned")) {
				return "V";
			} else {
				return "J";
			}
		} else if (CxxUtilities::String::include(dataTypeName, "long")) {
			return "K";
		} else if (CxxUtilities::String::include(dataTypeName, "float")) {
			return "E";
		} else if (CxxUtilities::String::include(dataTypeName, "double")) {
			return "D";
		} else if (CxxUtilities::String::include(dataTypeName, "x") || CxxUtilities::String::include(dataTypeName, "bits")
				|| CxxUtilities::String::include(dataTypeName, "bit")) {
			std::string dataTypeWidth = CxxUtilities::String::replace(dataTypeName, "x", "");
			dataTypeWidth = CxxUtilities::String::replace(dataTypeWidth, "bits", "");
			dataTypeWidth = CxxUtilities::String::replace(dataTypeWidth, "bit", "");
			dataTypeWidth = CxxUtilities::String::replace(dataTypeWidth, " ", "");
			return dataTypeWidth + "X";
		} else {
			using namespace std;
			cerr
					<< "CxxUtilities::FitsUtility::convertSIB2StyleDataTypeNameToCfitsioStyleTFORM(): Unrecognizable data type name "
					<< dataTypeName << " provided." << endl;
			return "";
		}
	} // end of convertSIB2StyleDataTypeNameToCfitsioStyleTFORM()

	static int getCfitsioStyleDataTypeNumber(std::string dataTypeName) {
		dataTypeName = CxxUtilities::String::downCase(dataTypeName);
		if (CxxUtilities::String::include(dataTypeName, "byte")) {
			if (CxxUtilities::String::include(dataTypeName, "unsigned")) {
				return TBYTE;
			} else {
				return TSBYTE;
			}
		} else if (CxxUtilities::String::include(dataTypeName, "short")) {
			if (CxxUtilities::String::include(dataTypeName, "unsigned")) {
				return TUSHORT;
			} else {
				return TSHORT;
			}
		} else if (CxxUtilities::String::include(dataTypeName, "int")) {
			if (CxxUtilities::String::include(dataTypeName, "unsigned")) {
				return TUINT;
			} else {
				return TINT;
			}
		} else if (CxxUtilities::String::include(dataTypeName, "long")) {
			return TLONG;
		} else if (CxxUtilities::String::include(dataTypeName, "float")) {
			return TFLOAT;
		} else if (CxxUtilities::String::include(dataTypeName, "double")) {
			return TDOUBLE;
		} else if (CxxUtilities::String::include(dataTypeName, "x") || CxxUtilities::String::include(dataTypeName, "bits")
				|| CxxUtilities::String::include(dataTypeName, "bit")) {
			return TBIT;
		} else {
			using namespace std;
			cerr << "CxxUtilities::FitsUtility::getCfitsioStyleDataTypeNumber(): Unrecognizable data type name "
					<< dataTypeName << " provided." << endl;
			return 0;
		}
	}

	static std::string getCfitsioStyleDataTypeNumberAsString(std::string dataTypeName) {
		dataTypeName = CxxUtilities::String::downCase(dataTypeName);
		if (CxxUtilities::String::include(dataTypeName, "byte")) {
			if (CxxUtilities::String::include(dataTypeName, "unsigned")) {
				return "TBYTE";
			} else {
				return "TSBYTE";
			}
		} else if (CxxUtilities::String::include(dataTypeName, "short")) {
			if (CxxUtilities::String::include(dataTypeName, "unsigned")) {
				return "TUSHORT";
			} else {
				return "TSHORT";
			}
		} else if (CxxUtilities::String::include(dataTypeName, "int")) {
			if (CxxUtilities::String::include(dataTypeName, "unsigned")) {
				return "TUINT";
			} else {
				return "TINT";
			}
		} else if (CxxUtilities::String::include(dataTypeName, "long")) {
			return "TLONG";
		} else if (CxxUtilities::String::include(dataTypeName, "float")) {
			return "TFLOAT";
		} else if (CxxUtilities::String::include(dataTypeName, "double")) {
			return "TDOUBLE";
		} else if (CxxUtilities::String::include(dataTypeName, "x") || CxxUtilities::String::include(dataTypeName, "bits")
				|| CxxUtilities::String::include(dataTypeName, "bit")) {
			return "TBIT";
		} else {
			using namespace std;
			cerr << "CxxUtilities::FitsUtility::getCfitsioStyleDataTypeNumber(): Unrecognizable data type name "
					<< dataTypeName << " provided." << endl;
			return "INVALID";
		}
	}

	/**
	 * SIB2-style data type names are:
	 * byte,unsignedByte
	 * short,unsignedShort
	 * int,unsignedInt
	 * long,unsignedLong
	 * float
	 * double
	 * hexBinary
	 */

	static void getTZEROAndTSCALForSIB2StyleDataTypeName(std::string dataTypeName, double& resultingTZERO,
			double& resultingTSCAL) {
		char type = convertSIB2StyleDataTypeNameToCfitsioStyleTFORM(dataTypeName)[0];
		dataTypeName = CxxUtilities::String::downCase(dataTypeName);
		if (CxxUtilities::String::include(dataTypeName, "float") || CxxUtilities::String::include(dataTypeName, "double")
				|| CxxUtilities::String::include(dataTypeName, "x") || CxxUtilities::String::include(dataTypeName, "bit")) {
			//nothing to do
			return;
		}
		if (CxxUtilities::String::include(dataTypeName, "unsigned")) { //unsigned
			switch (type) {
			case 'B': //unsiged byte
				resultingTSCAL = 1;
				resultingTZERO = 0;
				break;
			case 'I': //unsigned short
			case 'U':
				resultingTSCAL = 1;
				resultingTZERO = 32768;
				break;
			case 'J': //unsigned int
			case 'V':
				resultingTSCAL = 1;
				resultingTZERO = 2147483648;
				break;
			case 'K': //unsigned long
				resultingTSCAL = 1;
				resultingTZERO = 9223372036854775808;
				break;
			default:
				break;
			}
		} else { //signed
			switch (type) {
			case 'B': //signed byte
			case 'S':
				resultingTSCAL = 1;
				resultingTZERO = -128;
				break;
			case 'I': //signed short
				resultingTSCAL = 1;
				resultingTZERO = 0;
				break;
			case 'J': //signed int
				resultingTSCAL = 1;
				resultingTZERO = 0;
				break;
			case 'K': //signed long
				resultingTSCAL = 1;
				resultingTZERO = 0;
				break;
			default:
				using namespace std;
				cerr << "CxxUtilities::FitsUtility::getTZEROAndTSCALForSIB2StyleDataTypeName(): Unrecognizable data type name "
						<< dataTypeName << " provided." << endl;
				break;
			}
		}
	} //end of getTZEROAndTSCALForSIB2StyleDataTypeName()
};

class FitsColumnDefinition {
public:
	/** Column name.
	 */
	std::string TTYPE;

public:
	/** Column data type.
	 */
	std::string TFORM;

public:
	std::string dataTypeName;

public:
	/** Unit.
	 */
	std::string TUNIT;

public:
	/** Column display format.
	 */
	std::string TDISP;

public:
	/** Scaling factor.
	 */
	double TSCAL = 0;

public:
	/** Offset used in the scaling.
	 */
	double TZERO = 0;

public:
	/** Comment.
	 */
	std::string comment;

private:
	bool isUnitSet_ = false;
	bool isDisplayFormatSet_ = false;

public:
	FitsColumnDefinition(std::string columnName, std::string columnDataType, std::string comment = "") {
		this->TTYPE = columnName;
		this->dataTypeName = columnDataType;
		std::string cfitsioStyleTFORM = FitsUtility::convertSIB2StyleDataTypeNameToCfitsioStyleTFORM(columnDataType);
		if (CxxUtilities::String::containsNumber(cfitsioStyleTFORM)) {
			this->TFORM = cfitsioStyleTFORM;
		} else {
			this->TFORM = "1" + cfitsioStyleTFORM;
		}
		this->comment = comment;
		setTZEROAndTSCAL();
	}

public:
	void setTZEROAndTSCAL() {
		FitsUtility::getTZEROAndTSCALForSIB2StyleDataTypeName(this->dataTypeName, this->TZERO, this->TSCAL);
	}

public:
	void setUnit(std::string unit) {
		this->TUNIT = unit;
		this->isUnitSet_ = true;
	}

public:
	bool isUnitSet() {
		return isUnitSet_;
	}

public:
	void setDisplayFormat(std::string format) {
		this->TDISP = format;
		this->isDisplayFormatSet_ = true;
	}

public:
	bool isDisplayFormatSet() {
		return isDisplayFormatSet_;
	}

};

class FitsExtensionDefinition {
public:
	std::string extensionName;

public:
	std::vector<FitsColumnDefinition> columnDefinitions;

public:
	FitsExtensionDefinition(std::string extensionName) {
		this->extensionName = extensionName;
	}

public:
	FitsExtensionDefinition(std::string extensionName, std::vector<FitsColumnDefinition> columnDefinitions) {
		this->extensionName = extensionName;
		this->columnDefinitions = columnDefinitions;
	}

public:
	void push(const FitsColumnDefinition& columnDefinition) {
		this->columnDefinitions.push_back(columnDefinition);
	}

public:
	FitsColumnDefinition& at(size_t index) {
		return this->columnDefinitions.at(index);
	}

public:
	size_t nColumns() {
		return this->columnDefinitions.size();
	}

public:
	size_t size() {
		return this->columnDefinitions.size();
	}

public:
	static FitsExtensionDefinition* constructFromListOfColumnNameDataTypeAndComment(std::string extensionName,
			std::string listOfColumnNamesAndDataTypeNames, std::string delimiter = " ") {
		FitsExtensionDefinition* extensionDefinition = new FitsExtensionDefinition(extensionName);
		auto lines = CxxUtilities::String::splitIntoLines(listOfColumnNamesAndDataTypeNames);
		for (auto& line : lines) {
			std::vector<std::string> array = CxxUtilities::String::split(line, delimiter);
			if (array.size() < 2) {
				using namespace std;
				cerr << "CxxUtilities::FitsExtensionDefinition::constructFromListOfColumnNameDataTypeAndComment(): "
						<< "invalid entry in the column definition list (" << line << ")" << endl;
				continue;
			}
			std::string columnName = array[0];
			std::string columnDataType = array[1];
			std::string comment = "";
			if (array.size() >= 3) {
				std::stringstream ss;
				for (size_t i = 2; i < array.size(); i++) {
					ss << array[i];
					if (i != array.size() - 1) {
						ss << " ";
					}
				}
				comment = ss.str();
			}
			FitsColumnDefinition columnDefinition(columnName, columnDataType, comment);
			extensionDefinition->push(columnDefinition);
		}
		return extensionDefinition;
	}

public:
	char** createTTYPEs() {
		char** ttypes = new char*[this->columnDefinitions.size()];
		for (size_t i = 0; i < this->columnDefinitions.size(); i++) {
			ttypes[i] = new char[this->columnDefinitions.at(i).TTYPE.size() + 1];
			strcpy(ttypes[i], this->columnDefinitions.at(i).TTYPE.c_str());
		}
		return ttypes;
	}

public:
	void deleteTTYPEs(char** ttypes) {
		for (size_t i = 0; i < this->columnDefinitions.size(); i++) {
			delete ttypes[i];
		}
		delete ttypes;
	}

public:
	char** createTFORMs() {
		char** tforms = new char*[this->columnDefinitions.size()];
		for (size_t i = 0; i < this->columnDefinitions.size(); i++) {
			tforms[i] = new char[this->columnDefinitions.at(i).TFORM.size() + 1];
			strcpy(tforms[i], this->columnDefinitions.at(i).TFORM.c_str());
		}
		return tforms;
	}

public:
	void deleteTFORMs(char** tforms) {
		for (size_t i = 0; i < this->columnDefinitions.size(); i++) {
			delete tforms[i];
		}
		delete tforms;
	}

public:
	char** createTUNITs() {
		char** tunits = new char*[this->columnDefinitions.size()];
		for (size_t i = 0; i < this->columnDefinitions.size(); i++) {
			tunits[i] = new char[this->columnDefinitions.at(i).TUNIT.size() + 1];
			strcpy(tunits[i], this->columnDefinitions.at(i).TUNIT.c_str());
		}
		return tunits;
	}

public:
	void deleteTUNITs(char** tunits) {
		for (size_t i = 0; i < this->columnDefinitions.size(); i++) {
			delete tunits[i];
		}
		delete tunits;
	}

public:
	const std::string& getExtensionName() const {
		return extensionName;
	}

public:
	void setExtensionName(const std::string& extensionName) {
		this->extensionName = extensionName;
	}

private:
	std::vector<std::string> columnNames;
	std::map<std::string, size_t> columnNameAndColumnIndexMap;
	std::vector<std::tuple<std::string/*column name*/, int/*column index*/, int/*data type number*/>> columnNameAndColumnIndexVector;

public:
	std::vector<std::string>& getColumnNames() {
		if (this->columnNames.size() != this->columnDefinitions.size()) {
			this->columnNames.clear();
			for (auto& e : this->columnDefinitions) {
				this->columnNames.push_back(e.TTYPE);
			}
		}
		return columnNames;
	}

public:
	size_t getColumnIndex(std::string columnName) {
		if (this->columnNameAndColumnIndexMap.size() != this->columnDefinitions.size()) {
			this->columnNameAndColumnIndexMap.clear();
			for (size_t i = 0; i < this->columnDefinitions.size(); i++) {
				auto columnName = this->columnDefinitions[i].TTYPE;
				this->columnNameAndColumnIndexMap[columnName] = i;
			}
		}
		if (columnNameAndColumnIndexMap.count(columnName) != 0) {
			return columnNameAndColumnIndexMap[columnName];
		} else {
			using namespace std;
			cerr << "CxxUtilities::FitsExtensionDefinition::getColumnIndex(): invalid column name " << columnName << endl;
			exit(-1);
		}
	}

public:
	std::vector< //
			std::tuple<std::string/*column name*/, int/*column index*/, int/*data type number*/> //
	>& getColumnNameAndColumnIndexTupleVector() {
		if (this->columnNameAndColumnIndexVector.size() != this->columnDefinitions.size()) {
			this->columnNameAndColumnIndexVector.clear();
			for (size_t i = 0; i < this->columnDefinitions.size(); i++) {
				auto columnName = this->columnDefinitions[i].TTYPE;
				auto columnIndex = i;
				auto dataTypeNumber = FitsUtility::getCfitsioStyleDataTypeNumber(this->columnDefinitions[i].dataTypeName);
				std::tuple<std::string, int, int> entry = std::make_tuple(columnName, columnIndex, dataTypeNumber);
				this->columnNameAndColumnIndexVector.push_back(entry);
			}
		}
		return this->columnNameAndColumnIndexVector;
	}

public:
	std::string toString() {
		using namespace std;
		std::stringstream ss;
		size_t i = 0;
		for (auto& e : this->columnDefinitions) {
			ss << left << setw(3) << i << " TTYPE=" << setw(32) << e.TTYPE << " TFORM=" << setw(4) << e.TFORM << " TUNIT="
					<< setw(5) << e.TUNIT << " TZERO=" << setw(12) << e.TZERO << " TSCAL=" << setw(12) << e.TSCAL << " "
					<< setw(15) << e.dataTypeName << " " << setw(8)
					<< FitsUtility::getCfitsioStyleDataTypeNumberAsString(e.dataTypeName) << endl;
			i++;
		}
		return ss.str();
	}
};

}
#endif /* CXXUTILITIES_FITSFILE_HH_ */
