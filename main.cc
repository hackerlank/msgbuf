#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdint.h>

std::string Trim(const std::string& str)
{
    std::string result;
    std::string::size_type pos1 = 0, pos2 = str.length();

    if(pos2 == 0)
        return str;

    while(pos1 < str.size() && (str[pos1] == ' ' || str[pos1] == '\t' || str[pos1] == '\r' || str[pos1] == '\n'))
        ++pos1;

    pos2--;
    while(pos2 > 0 && (str[pos2] == ' ' || str[pos2] == '\t' || str[pos2] == '\r' || str[pos2] == '\n'))
        --pos2;

    return str.substr(pos1, pos2 - pos1 + 1);
}

    template <class T>
T ToNumber(const std::string& str)
{
    std::istringstream ss(str);
    T value;
    ss >> value;

    return value;
}

    template <class T>
std::string ToString(const T& v)
{
    std::ostringstream ss;
    ss << v;
    return ss.str();
}

std::string MangleInternalKeyword(const std::string& keyword)
{
    return "mangledMsgBUFKeyword_" + keyword;
}


struct DataMember
{
    std::string type;
    std::string nativeType;
    std::string name;
    int count;

    DataMember() : count(1) {}
};

struct Message
{
    std::string name;

    typedef std::vector<DataMember> MemberList;
    MemberList memberList;
};

struct Options
{
    uint32_t version;
    std::string package;
    std::string baseclass;

    Options() : version(0), package("Msg"), baseclass("Message") {}
};

typedef std::vector<Message> MessageList;

struct Type
{
    const char* type;
    const char* nativeType;
    int typeSize;
    int storageOverhead;
};

Type typeList[] = {
    {"i8", "int8_t", 1, 0},
    {"u8", "uint8_t", 1, 0},
    {"i16", "int16_t", 2, 0},
    {"u16", "uint16_t", 2, 0},
    {"i32", "int32_t", 4, 0},
    {"u32", "uint32_t", 4, 0},
    {"i64", "int64_t", 8, 0},
    {"u64", "uint64_t", 8, 0},
    {"float", "float", 4, 0},
    {"double", "double", 8, 0},
    {"str", "std::string", 0, 4},
};

std::string ConvertType(const std::string& type)
{
    for(int i = 0; i < sizeof(typeList) / sizeof(Type); ++i)
    {
        if(std::string(typeList[i].type) == type)
            return typeList[i].nativeType;
    }

    return "";
}

int GetTypeStorageSize(const std::string& type)
{
    for(int i = 0; i < sizeof(typeList) / sizeof(Type); ++i)
    {
        if(std::string(typeList[i].type) == type)
            return typeList[i].typeSize + typeList[i].storageOverhead;
    }

    return 0;
}

int GetTypeSize(const std::string& type)
{
    for(int i = 0; i < sizeof(typeList) / sizeof(Type); ++i)
    {
        if(std::string(typeList[i].type) == type)
            return typeList[i].typeSize;
    }

    return 0;
}

std::string GetSwapByteSuffix(const std::string& type)
{
    std::string swapByteSuffix = ToString(GetTypeSize(type));
    if(type == "float" || type == "double")
        swapByteSuffix += "f";

    return swapByteSuffix;
}

bool ProcessInputFile(const std::string& filename, MessageList& messageList, Options& options)
{
    std::ifstream f(filename.c_str());
    if(!f.is_open())
        return false;

    Message currentMessage;

    std::string line;
    while(std::getline(f, line))
    {
        line = Trim(line);

        if(line.length() == 0)
            continue;

        if(line[0] == '#')
        {
            // Skip comment
            continue;
        }
        else if(line[0] == '.')
        {
            if(currentMessage.name != "")
                messageList.push_back(currentMessage);

            currentMessage = Message();
            currentMessage.name = line.substr(1);
        }
        else if(line[0] == '!')
        {
            std::string::size_type pos = line.find(' ');
            if(pos == std::string::npos)
            {
                std::cout << "Invalid option line: " << line << std::endl;
                return false;
            }

            std::string option = Trim(line.substr(1, pos - 1));
            std::string value = Trim(line.substr(pos + 1));

            std::cout << "'" << option << "' = '" << value << "'" << std::endl;

            if(option == "version")
                options.version = ToNumber<uint32_t>(Trim(value));
            else if(option == "package")
                options.package = Trim(value);
            else if(option == "baseclass")
                options.baseclass = Trim(value);
        }
        else
        {
            if(currentMessage.name == "")
            {
                std::cout << "Invalid, not in message block" << std::endl;
                return false;
            }

            std::string::size_type pos = line.rfind('\t');
            if(pos == std::string::npos)
                pos = line.rfind(' ');
            if(pos == std::string::npos)
            {
                std::cout << "Invalid line: " << line << std::endl;
                return false;
            }

            DataMember dataMember;
            dataMember.name = Trim(line.substr(pos));

            std::string::size_type posLen1 = line.find('[');
            if(posLen1 != std::string::npos)
            {
                pos = posLen1;

                posLen1++;
                std::string::size_type posLen2 = line.find(']');
                if(posLen2 == std::string::npos)
                {
                    std::cout << "Invalid line, bad array size syntax: " << line << std::endl;
                    return false;
                }

                std::string sizeStr = line.substr(posLen1, posLen2 - posLen1);
                dataMember.count = ToNumber<int>(sizeStr);
                if(dataMember.count == 0)
                {
                    std::cout << "Invalid line, bad array size (" << sizeStr << ")" << std::endl;
                    return false;
                }
            }

            dataMember.type = Trim(line.substr(0, pos));
            dataMember.nativeType = ConvertType(dataMember.type);
            if(dataMember.nativeType == "")
            {
                std::cout << "Invalid type on line: " << line << std::endl;
                return false;
            }

            currentMessage.memberList.push_back(dataMember);
        }

        std::cout << line << std::endl;
    }

    if(currentMessage.name != "")
        messageList.push_back(currentMessage);

    f.close();
    return true;
}

bool WriteOutput(const std::string& output, const MessageList& messageList, const Options& options)
{
    std::ofstream f(output.c_str());
    if(!f.is_open())
        return false;

    std::string includeGuard = "MSGBUF_" + options.package + "_" + options.baseclass + "_" + ToString(options.version) + "_INCLUDED__";
    f << "#ifndef " << includeGuard << std::endl;
    f << "#define " << includeGuard << std::endl;

    f << std::endl;
    f << "// DO NOT MODIFY, THIS FILE WAS AUTO-GENERATED BY MSGBUF" << std::endl;

    f << std::endl;
    f << "#include <string>" << std::endl;
    f << "#include <cstring>" << std::endl;
    f << "#include <stdint.h>" << std::endl;

    f << std::endl;
    f << "namespace " << options.package << " {" << std::endl;

    f << std::endl;
    f << "class " << options.baseclass << "Factory;" << std::endl;
    f << std::endl;

    f << "class " << options.baseclass << std::endl;
    f << "{" << std::endl;
    f << "    public:" << std::endl;
    f << "        enum MESSAGE_TYPE {";
    for(int i = 0; i < messageList.size(); ++i)
    {
        const Message& msg = messageList[i];
        f << "MT_" << msg.name;
        if(i != messageList.size() - 1)
            f << ", ";
    }
    f << "};" << std::endl;
    f << "    public:" << std::endl;
    f << "        virtual MESSAGE_TYPE GetType() const = 0;" << std::endl;
    f << "        // Output to a user-allocated buffer, make sure there is enough" << std::endl;
    f << "        // len must be greater or equal to the value returned by CalculateNeededSerializationSize)" << std::endl;
    f << "        // returns the length stored in buffer" << std::endl;
    f << "        uint32_t ToBuffer(char* buffer, uint32_t len) const" << std::endl;
    f << "        {" << std::endl;
    f << "             return InternalSerializeToBuffer(buffer, len);" << std::endl;
    f << "        }" << std::endl;
    f << "        std::string ToStringBuffer() const" << std::endl;
    f << "        {" << std::endl;
    f << "             return InternalSerializeToStringBuffer();" << std::endl;
    f << "        }" << std::endl;
    f << "        static uint32_t GetVersion()" << std::endl;
    f << "        {" << std::endl;
    f << "             return " << options.version << ";" << std::endl;
    f << "        }" << std::endl;
    f << "        uint32_t CalculateNeededSerializationSize() const" << std::endl;
    f << "        {" << std::endl;
    f << "             return InternalCalculateNeededSerializationSize();" << std::endl;
    f << "        }" << std::endl;
    f << "    protected:" << std::endl;
    f << "        virtual uint32_t InternalSerializeToBuffer(char* p, uint32_t len) const = 0;" << std::endl;
    f << "        virtual std::string InternalSerializeToStringBuffer() const = 0;" << std::endl;
    f << "        virtual void InternalCreateFromBuffer(const char* p) = 0;" << std::endl;
    f << "        virtual uint32_t InternalCalculateNeededSerializationSize() const = 0;" << std::endl;
    f << "    protected:" << std::endl;
    f << "        static bool ShouldSwap()" << std::endl;
    f << "        {" << std::endl;
    f << "             static const uint16_t swapTest = 1;" << std::endl;
    f << "             return (*((char*)&swapTest) == 1);" << std::endl;
    f << "        }" << std::endl;
    f << "        static uint16_t SwapByte2(uint16_t v)" << std::endl;
    f << "        {" << std::endl;
    f << "             if(ShouldSwap())" << std::endl;
    f << "                 return (v >> 8) | (v << 8);" << std::endl;
    f << "             return v;" << std::endl;
    f << "        }" << std::endl;
    f << "        static uint32_t SwapByte4(uint32_t v)" << std::endl;
    f << "        {" << std::endl;
    f << "             if(ShouldSwap())" << std::endl;
    f << "                 return (SwapByte2(v & 0xffff) << 16) | (SwapByte2( v >> 16));" << std::endl;
    f << "             return v;" << std::endl;
    f << "        }" << std::endl;
    f << "        static uint64_t SwapByte8(uint64_t v)" << std::endl;
    f << "        {" << std::endl;
    f << "             if(ShouldSwap())" << std::endl;
    f << "                 return (((uint64_t)SwapByte4((uint32_t)(v & 0xffffffffull))) << 32) | (SwapByte4((uint32_t)(v >> 32)));" << std::endl;
    f << "             return v;" << std::endl;
    f << "        }" << std::endl;
    f << "        static void SwapBytes(uint8_t& v1, uint8_t& v2)" << std::endl;
    f << "        {" << std::endl;
    f << "            uint8_t tmp = v1;" << std::endl;
    f << "            v1 = v2;" << std::endl;
    f << "            v2 = tmp;" << std::endl;
    f << "        }" << std::endl;
    f << "        static float SwapByte4f(float v)" << std::endl;
    f << "        {" << std::endl;
    f << "             union { float f; uint8_t c[4]; };" << std::endl;
    f << "             f = v;" << std::endl;
    f << "             if(ShouldSwap())" << std::endl;
    f << "             {" << std::endl;
    f << "                 SwapBytes(c[0], c[3]);" << std::endl;
    f << "                 SwapBytes(c[1], c[2]);" << std::endl;
    f << "             }" << std::endl;
    f << "             return f;" << std::endl;
    f << "        }" << std::endl;
    f << "        static double SwapByte8f(double v)" << std::endl;
    f << "        {" << std::endl;
    f << "             union { double f; uint8_t c[8]; };" << std::endl;
    f << "             f = v;" << std::endl;
    f << "             if(ShouldSwap())" << std::endl;
    f << "             {" << std::endl;
    f << "                 SwapBytes(c[0], c[7]);" << std::endl;
    f << "                 SwapBytes(c[1], c[6]);" << std::endl;
    f << "                 SwapBytes(c[2], c[5]);" << std::endl;
    f << "                 SwapBytes(c[3], c[4]);" << std::endl;
    f << "             }" << std::endl;
    f << "             return f;" << std::endl;
    f << "        }" << std::endl;
    f << "    friend class " << options.baseclass << "Factory;" << std::endl;
    f << "};" << std::endl;









    for(int i = 0; i < messageList.size(); ++i)
    {
        const Message& msg = messageList[i];
        f << "class " << msg.name << " : public " << options.baseclass << std::endl;
        f << "{" << std::endl;

        f << "    public:" << std::endl;
        for(int j = 0; j < msg.memberList.size(); ++j)
        {
            const DataMember& member = msg.memberList[j];
            f << "        " << member.nativeType << " " << member.name;
            if(member.count != 1)
                f << "[" << member.count << "]";
            f << ";" << std::endl;
        }
        f << "        " << msg.name << "() {}" << std::endl;
        std::string ctorParams;
        std::string ctorInitializer;
        for(int j = 0; j < msg.memberList.size(); ++j)
        {
            const DataMember& member = msg.memberList[j];
            if(member.count == 1)
                ctorParams += "const " + member.nativeType + "& n_" + MangleInternalKeyword(member.name);
            else
                ctorParams += "const " + member.nativeType + " n_" + MangleInternalKeyword(member.name) + "[" + ToString(member.count) + "]";
            if(member.count == 1)
            {
                ctorInitializer +=  member.name + "(n_" + MangleInternalKeyword(member.name) + ")";
                if(j != msg.memberList.size() - 1)
                    ctorInitializer += ", ";
            }
            if(j != msg.memberList.size() - 1)
                ctorParams += ", ";
        }
        f << "        " << msg.name << "(" << ctorParams << ") : " << ctorInitializer << std::endl;
        f << "        {" << std::endl;
        for(int j = 0; j < msg.memberList.size(); ++j)
        {
            const DataMember& member = msg.memberList[j];
            if(member.count > 1)
            {
                f << "             for(int " << MangleInternalKeyword("i") << " = 0; " << MangleInternalKeyword("i") << " < " << member.count << "; ++" << MangleInternalKeyword("i") << ")" << std::endl;
                f << "                 " << member.name << "[" << MangleInternalKeyword("i") << "] = n_" << MangleInternalKeyword(member.name) << "[" << MangleInternalKeyword("i") << "];" << std::endl;
            }
        }
        f << "        }" << std::endl;
        //---------------------------------------------------------------------
        f << "        virtual " << options.baseclass << "::MESSAGE_TYPE GetType() const { return " << options.baseclass << "::MT_" << msg.name << "; }" << std::endl;
        f << "    protected:" << std::endl;

        //---------------------------------------------------------------------
        f << "        virtual uint32_t InternalSerializeToBuffer(char* " << MangleInternalKeyword("p") << ", uint32_t " << MangleInternalKeyword("len") << ") const" << std::endl;
        f << "        {" << std::endl;
        f << "             uint32_t " << MangleInternalKeyword("storageSize") << " = InternalCalculateNeededSerializationSize();" << std::endl;
        f << "             if(" << MangleInternalKeyword("len") << " < " << MangleInternalKeyword("storageSize") << ")" << std::endl;
        f << "                 return 0;" << std::endl;
        // Version and message type
        f << "             // Version" << std::endl;
        //f << "             uint32_t version = GetVersion();" << std::endl;
        f << "             *((uint32_t*)" << MangleInternalKeyword("p") << ") = SwapByte4((uint32_t)GetVersion());" << std::endl;
        f << "             " << MangleInternalKeyword("p") << " += 4;" << std::endl;
        f << "             // Message type" << std::endl;
        //f << "             uint32_t type = GetType();" << std::endl;
        f << "             *((uint32_t*)" << MangleInternalKeyword("p") << ") = SwapByte4((uint32_t)GetType());" << std::endl;
        f << "             " << MangleInternalKeyword("p") << " += 4;" << std::endl;
        for(int j = 0; j < msg.memberList.size(); ++j)
        {
            const DataMember& member = msg.memberList[j];
            f << "             // " << member.name << std::endl;
            if(member.count > 1)
            {
                f << "             for(int " << MangleInternalKeyword("i") << " = 0; " << MangleInternalKeyword("i") << " < " << member.count << "; ++" << MangleInternalKeyword("i") << ")" << std::endl;
                f << "             {" << std::endl;
            }
            if(member.type == "str")
            {
                //f << "             uint32_t len" << " = " << member.name << (member.count > 1 ? "[i]" : "") << ".length();" << std::endl;
                f << "             *((uint32_t*)" << MangleInternalKeyword("p") << ") = SwapByte4((uint32_t)" << member.name << (member.count > 1 ? "[" + MangleInternalKeyword("i") + "]" : "") << ".length()" << ");" << std::endl;
                f << "             " << MangleInternalKeyword("p") << " += 4;" << std::endl;
                f << "             memcpy(" << MangleInternalKeyword("p") << ", " << member.name << (member.count > 1 ? "[" + MangleInternalKeyword("i") + "]" : "") << ".c_str(), " << member.name << (member.count > 1 ? "[" + MangleInternalKeyword("i") + "]" : "") << ".length());" << std::endl;
            }
            else if(GetTypeStorageSize(member.type) == 1)
                f << "             *" << MangleInternalKeyword("p") << " = *((char*)&" << member.name << (member.count > 1 ? "[" + MangleInternalKeyword("i") + "]" : "") << ");" << std::endl;
            else
            {
                f << "            *((" << member.nativeType << "*)" << MangleInternalKeyword("p") << ") = (" << member.nativeType << ")SwapByte" << GetSwapByteSuffix(member.type) << "(" << member.name << (member.count > 1 ? "[" + MangleInternalKeyword("i") + "]" : "") << ");" << std::endl;
            }
            if(j != msg.memberList.size() - 1)
            {
                if(member.type == "str")
                    f << "             " << MangleInternalKeyword("p") << " += " << member.name + (member.count > 1 ? "[" + MangleInternalKeyword("i") + "]" : "") + ".length()" << ";" << std::endl;
                else
                    f << "             " << MangleInternalKeyword("p") << " += " << GetTypeStorageSize(member.type) << ";" << std::endl;
            }
            if(member.count > 1)
            {
                f << "             }" << std::endl;
            }
        }
        f << "             return " << MangleInternalKeyword("storageSize") << ";" << std::endl;
        f << "        }" << std::endl;

        //---------------------------------------------------------------------
        f << "        virtual std::string InternalSerializeToStringBuffer() const" << std::endl;
        f << "        {" << std::endl;
        f << "             int " << MangleInternalKeyword("storageSize") << "= InternalCalculateNeededSerializationSize();" << std::endl;
        f << "             char* " << MangleInternalKeyword("buffer") << " = new char[" << MangleInternalKeyword("storageSize") << "];" << std::endl;
        f << "             InternalSerializeToBuffer(" << MangleInternalKeyword("buffer") << ", " << MangleInternalKeyword("storageSize") << ");" << std::endl;
        f << "             std::string " << MangleInternalKeyword("result") << "(" << MangleInternalKeyword("buffer") << ", " << MangleInternalKeyword("storageSize") << ");" << std::endl;
        f << "             delete [] " << MangleInternalKeyword("buffer") << ";" << std::endl;
        f << "             return " << MangleInternalKeyword("result") << ";" << std::endl;
        f << "        }" << std::endl;


        //---------------------------------------------------------------------
        f << "        virtual void InternalCreateFromBuffer(const char* " << MangleInternalKeyword("p") << ")" << std::endl;
        f << "        {" << std::endl;
        for(int j = 0; j < msg.memberList.size(); ++j)
        {
            const DataMember& member = msg.memberList[j];
            f << "             // " << member.name << std::endl;
            if(member.count > 1)
            {
                f << "             for(int " << MangleInternalKeyword("i") << " = 0; " << MangleInternalKeyword("i") << " < " << member.count << "; ++" << MangleInternalKeyword("i") << ")" << std::endl;
                f << "             {" << std::endl;
            }
            if(member.type == "str")
            {

                f << "             uint32_t len_" << member.name << " = SwapByte4(*(uint32_t*)" << MangleInternalKeyword("p") << ");" << std::endl;
                f << "             " << MangleInternalKeyword("p") << " += 4;" << std::endl;
                f << "             " << member.name << (member.count > 1 ? "[" + MangleInternalKeyword("i") + "]" : "") << " = std::string(" << MangleInternalKeyword("p") << ", len_" << member.name << ");" << std::endl;
                if(j != msg.memberList.size() - 1)
                    f << "             " << MangleInternalKeyword("p") << " += len_" << member.name << ";" << std::endl;
            }
            else if(GetTypeStorageSize(member.type) == 1)
            {
                f << "             " << member.name << (member.count > 1 ? "[" + MangleInternalKeyword("i") + "]" : "") << " = (" << member.nativeType << ")*" << MangleInternalKeyword("p") << ";" << std::endl;
                if(j != msg.memberList.size() - 1)
                    f << "             " << MangleInternalKeyword("p") << "++;" << std::endl;
            }
            else
            {
                f << "             " << member.name << (member.count > 1 ? "[" + MangleInternalKeyword("i") + "]" : "") << " = (" << member.nativeType << ")SwapByte" << GetSwapByteSuffix(member.type) << "(*(" << member.nativeType << "*)" << MangleInternalKeyword("p") << ");" << std::endl;
                if(j != msg.memberList.size() - 1)
                    f << "             " << MangleInternalKeyword("p") << " += " << GetTypeStorageSize(member.type) << ";" << std::endl;
            }
            if(member.count > 1)
            {
                f << "             }" << std::endl;
            }
        }
        f << "        }" << std::endl;

        //---------------------------------------------------------------------
        f << "        virtual uint32_t InternalCalculateNeededSerializationSize() const" << std::endl;
        f << "        {" << std::endl;
        int size = 8; // version(4  bytes) and message type (4 bytes)
        for(int j = 0; j < msg.memberList.size(); ++j)
        {
            const DataMember& member = msg.memberList[j];
            size += GetTypeStorageSize(member.type) * member.count;
        }
        f << "             int " << MangleInternalKeyword("size") << " = " << size << ";" << std::endl;
        for(int j = 0; j < msg.memberList.size(); ++j)
        {
            const DataMember& member = msg.memberList[j];
            if(member.type == "str")
            {
                if(member.count > 1)
                {
                    f << "             for(int " << MangleInternalKeyword("i") << " = 0; " << MangleInternalKeyword("i") << " < " << member.count << "; ++" << MangleInternalKeyword("i") << ")" << std::endl;
                    f << "             {" << std::endl;
                    f << "    ";
                }
                f << "             " << MangleInternalKeyword("size") << " += " << member.name << (member.count > 1 ? "[" + MangleInternalKeyword("i") + "]" : "") << ".length();" << std::endl;
                if(member.count > 1)
                {
                    f << "             }" << std::endl;
                }
            }
        }
        f << "             return " << MangleInternalKeyword("size") << ";" << std::endl;
        f << "        }" << std::endl;
        f << "};" << std::endl;
    }









    f << "class " << options.baseclass << "Factory" << std::endl;
    f << "{" << std::endl;
    f << "    public:" << std::endl;
    f << "        enum ERROR { NOERROR, INVALID_TYPE, BAD_VERSION };" << std::endl;
    f << "    public:" << std::endl;
    // Decode buffer and return a message
    //---------------------------------------------------------------------
    f << "        static " << options.baseclass << "* CreateFromBuffer(const char* buffer, ERROR* errorCode = 0)" << std::endl;
    f << "        {" << std::endl;
    f << "             uint32_t version = " << options.baseclass << "::SwapByte4(*((uint32_t*)buffer));" << std::endl;
    f << "             buffer += 4;" << std::endl;
    f << "             if(version != " << options.baseclass << "::GetVersion())" << std::endl;
    f << "             {" << std::endl;
    f << "                 if(errorCode)" << std::endl;
    f << "                     *errorCode = BAD_VERSION;" << std::endl;
    f << "                 return 0;" << std::endl;
    f << "             }" << std::endl;
    f << "             uint32_t type = " << options.baseclass << "::SwapByte4(*((uint32_t*)buffer));" << std::endl;
    f << "             buffer += 4;" << std::endl;
    f << "             " << options.baseclass << "* message = 0;" << std::endl;
    f << "             switch(type)" << std::endl;
    f << "             {" << std::endl;
    for(int i = 0; i < messageList.size(); ++i)
    {
        const Message& msg = messageList[i];
        f << "                 case " << options.baseclass << "::MT_" << msg.name << ":" <<  std::endl;
        f << "                     message = new " << msg.name << "();" <<  std::endl;
        f << "                     break;" <<  std::endl;
    }
    f << "             }" << std::endl;
    f << "             if(!message)" << std::endl;
    f << "             {" << std::endl;
    f << "                 if(errorCode)" << std::endl;
    f << "                     *errorCode = INVALID_TYPE;" << std::endl;
    f << "                 return 0;" << std::endl;
    f << "             }" << std::endl;
    f << "             message->InternalCreateFromBuffer(buffer);" << std::endl;
    f << "             if(errorCode)" << std::endl;
    f << "                 *errorCode = NOERROR;" << std::endl;
    f << "             return message;" << std::endl;
    f << "        }" << std::endl;

    //---------------------------------------------------------------------
    f << "        static " << options.baseclass << "* CreateFromStringBuffer(const std::string& buffer, ERROR* errorCode = 0)" << std::endl;
    f << "        {" << std::endl;
    f << "             return CreateFromBuffer(buffer.c_str(), errorCode);" << std::endl;
    f << "        }" << std::endl;
    f << "};" << std::endl;





    f << "}" << std::endl; // End of namespace
    f << "#endif // " << includeGuard << std::endl;


    f.close();
    return true;
}

int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        std::cout << "Usage: msgbuf <file.mb> <output.h>" << std::endl;
        return 1;
    }

    std::string input(argv[1]);
    std::string output(argv[2]);

    MessageList messageList;

    Options options;

    if(ProcessInputFile(input, messageList, options))
        WriteOutput(output, messageList, options);
}
