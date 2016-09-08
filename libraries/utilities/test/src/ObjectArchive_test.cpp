
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Machine Learning Library (EMLL)
//  File:     ObjectArchive_test.cpp (utilities)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ObjectArchive_test.h"

// utilities
#include "IArchivable.h"
#include "Archiver.h"
#include "XmlArchiver.h"
#include "ObjectArchiver.h"

// testing
#include "testing.h"

// stl
#include <iostream>
#include <vector>
#include <sstream>

class InnerObject : public utilities::IArchivable
{
public:
    InnerObject() = default;
    InnerObject(int a, double b) : _a(a), _b(b) {}
    int GetA() const { return _a; }
    double GetB() const { return _b; }

    virtual void WriteToArchive(utilities::Archiver& archiver) const override
    {
        archiver["a"] << _a;
        archiver["b"] << _b;
    }

    virtual void ReadFromArchive(utilities::Unarchiver& archiver) override
    {
        archiver["a"] >> _a;
        archiver["b"] >> _b;
    }

    static std::string GetTypeName() { return "InnerObject"; }
    virtual std::string GetRuntimeTypeName() const override { return GetTypeName(); }

private:
    int _a = 0;
    double _b = 0.0;
};

class DerivedObject : public InnerObject
{
public:
    DerivedObject() = default;
    DerivedObject(int a, double b, std::string c) : InnerObject(a, b), _c(c) {}
    std::string GetC() { return _c; }

    virtual void WriteToArchive(utilities::Archiver& archiver) const override
    {
        InnerObject::WriteToArchive(archiver);
        archiver["c"] << _c;
    }

    virtual void ReadFromArchive(utilities::Unarchiver& archiver) override
    {
        InnerObject::ReadFromArchive(archiver);
        archiver["c"] >> _c;
    }

    static std::string GetTypeName() { return "DerivedObject"; }
    virtual std::string GetRuntimeTypeName() const override { return GetTypeName(); }

private:
    std::string _c = "";
};

class OuterObject : public utilities::IArchivable
{
public:
    OuterObject() = default;
    OuterObject(std::string name, int a, double b) : _name(name), _inner(a, b) {}
    std::string GetName() { return _name; }
    InnerObject GetInner() { return _inner; }

    virtual void WriteToArchive(utilities::Archiver& archiver) const override
    {
        archiver["name"] << _name;
        archiver["obj"] << _inner;
    }

    virtual void ReadFromArchive(utilities::Unarchiver& archiver) override
    {
        archiver["name"] >> _name;
        archiver["obj"] >> _inner;
    }

    static std::string GetTypeName() { return "OuterObject"; }
    virtual std::string GetRuntimeTypeName() const override { return GetTypeName(); }

private:
    std::string _name;
    InnerObject _inner;
};

void PrintDescription(const utilities::ObjectArchive& description, std::string name="", size_t indentCount = 0)
{
    std::string indent(4*indentCount, ' ');
    std::cout << indent << name << " Type: " << description.GetObjectTypeName(); 
    if(description.HasValue())
    {
        std::cout << " = " << description.GetValueString();
    }
    std::cout << std::endl;    

    for(const auto& iter: description.GetProperties())
    {
        auto name = iter.first;
        auto prop = iter.second;
        PrintDescription(prop, name, indentCount+1);
    }
}

void TestGetTypeDescription()
{
    InnerObject innerObj;
    auto innerDescription = innerObj.GetDescription();
    PrintDescription(innerDescription);

    OuterObject outerObj;
    auto outerDescription = outerObj.GetDescription();
    PrintDescription(outerDescription);

    DerivedObject derivedObj;
    auto derivedDescription = derivedObj.GetDescription();
    PrintDescription(derivedDescription);

    testing::ProcessTest("GetDescription", innerDescription.HasProperty("a"));
    testing::ProcessTest("GetDescription", innerDescription.HasProperty("b"));
    testing::ProcessTest("GetDescription", !innerDescription.HasProperty("c"));

    testing::ProcessTest("GetDescription", outerDescription.HasProperty("name"));
    testing::ProcessTest("GetDescription", outerDescription.HasProperty("obj"));

    testing::ProcessTest("GetDescription", derivedDescription.HasProperty("a"));
    testing::ProcessTest("GetDescription", derivedDescription.HasProperty("b"));
    testing::ProcessTest("GetDescription", derivedDescription.HasProperty("c"));
}

void TestGetObjectArchive()
{
    InnerObject innerObj(3, 4.5);
    auto innerDescription = innerObj.GetDescription();
    PrintDescription(innerDescription);
    std::cout << std::endl;

    OuterObject outerObj("Outer", 5, 6.5);
    auto outerDescription = outerObj.GetDescription();
    PrintDescription(outerDescription);
    std::cout << std::endl;

    DerivedObject derivedObj(8, 9.5, "derived");
    auto derivedDescription = derivedObj.GetDescription();
    PrintDescription(derivedDescription);
    std::cout << std::endl;

    // Inner
    testing::ProcessTest("ObjectArchive", innerDescription.HasProperty("a"));
    testing::ProcessTest("ObjectArchive", innerDescription.HasProperty("b"));
    testing::ProcessTest("ObjectArchive", !innerDescription.HasProperty("c"));
    testing::ProcessTest("ObjectArchive", innerDescription["a"].GetValue<int>() == 3);
    testing::ProcessTest("ObjectArchive", innerDescription["b"].GetValue<double>() == 4.5);

    // Outer
    testing::ProcessTest("ObjectArchive", outerDescription.HasProperty("name"));
    testing::ProcessTest("ObjectArchive", outerDescription.HasProperty("obj"));
    testing::ProcessTest("ObjectArchive", outerDescription["name"].GetValue<std::string>() == "Outer");
    auto outerInnerDescription = outerDescription["obj"];
    testing::ProcessTest("ObjectArchive", outerInnerDescription["a"].GetValue<int>() == 5);
    testing::ProcessTest("ObjectArchive", outerInnerDescription["b"].GetValue<double>() == 6.5);

    // Derived
    testing::ProcessTest("ObjectArchive", derivedDescription.HasProperty("a"));
    testing::ProcessTest("ObjectArchive", derivedDescription.HasProperty("b"));
    testing::ProcessTest("ObjectArchive", derivedDescription.HasProperty("c"));
    testing::ProcessTest("ObjectArchive", derivedDescription["a"].GetValue<int>() == 8);
    testing::ProcessTest("ObjectArchive", derivedDescription["b"].GetValue<double>() == 9.5);
    testing::ProcessTest("ObjectArchive", derivedDescription["c"].GetValue<std::string>() == "derived");
}

void TestSerializeIArchivable()
{
    utilities::SerializationContext context;
    std::stringstream strstream;
    {
        utilities::XmlArchiver archiver(strstream);
     
        InnerObject innerObj(3, 4.5);
        archiver["inner"] << innerObj;

        OuterObject outerObj("Outer", 5, 6.5);
        archiver["outer"] << outerObj;

        DerivedObject derivedObj(8, 9.5, "derived");
        archiver["derived"] << derivedObj;

        // print
        std::cout << "Serialized stream:" << std::endl;
        std::cout << strstream.str() << std::endl;
    }

    utilities::XmlUnarchiver unarchiver(strstream, context);
    InnerObject deserializedInner;

    unarchiver["inner"] >> deserializedInner;
    testing::ProcessTest("Deserialize IArchivable check",  deserializedInner.GetA() == 3 && deserializedInner.GetB() == 4.5f);        

    OuterObject deserializedOuter;
    unarchiver["outer"] >> deserializedOuter;
    testing::ProcessTest("Deserialize IArchivable check",  deserializedOuter.GetName() == "Outer" && deserializedOuter.GetInner().GetA() == 5);        

    DerivedObject deserializedDerived;
    unarchiver["derived"] >> deserializedDerived;
    testing::ProcessTest("Deserialize IArchivable check",  deserializedDerived.GetA() == 8 && deserializedDerived.GetB() == 9.5 && deserializedDerived.GetC() == "derived");        
}

void TestObjectArchiver()
{
    utilities::SerializationContext context;
    utilities::ObjectArchiver archiver1(context);
    utilities::ObjectArchiver archiver2(context);
    utilities::ObjectArchiver archiver3(context);
    
    InnerObject innerObj(3, 4.5);
    archiver1 << innerObj;

    OuterObject outerObj("Outer", 5, 6.5);
    archiver2 << outerObj;

    DerivedObject derivedObj(8, 9.5, "derived");
    archiver3 << derivedObj;

    auto objectDescription1 = archiver1.GetObjectArchive();
    PrintDescription(objectDescription1);
    std::cout << std::endl;

    auto objectDescription2 = archiver2.GetObjectArchive();
    PrintDescription(objectDescription2);
    std::cout << std::endl;

    auto objectDescription3 = archiver3.GetObjectArchive();
    PrintDescription(objectDescription3);
    std::cout << std::endl;

    utilities::ObjectArchiver dearchiver1(objectDescription1, context);
    InnerObject deserializedInner;
    dearchiver1 >> deserializedInner;
    testing::ProcessTest("Deserialize with ObjectArchiver check",  deserializedInner.GetA() == 3 && deserializedInner.GetB() == 4.5f);        

    // TODO: fix error with deserializing compound objects
    utilities::ObjectArchiver dearchiver2(objectDescription2, context);
    OuterObject deserializedOuter;
    dearchiver2 >> deserializedOuter;
    testing::ProcessTest("Deserialize with ObjectArchiver check",  deserializedOuter.GetName() == "Outer" && deserializedOuter.GetInner().GetA() == 5);        

    utilities::ObjectArchiver dearchiver3(objectDescription3, context);
    DerivedObject deserializedDerived;
    dearchiver3 >> deserializedDerived;
    testing::ProcessTest("Deserialize with ObjectArchiver check",  deserializedDerived.GetA() == 8 && deserializedDerived.GetB() == 9.5 && deserializedDerived.GetC() == "derived");        
}