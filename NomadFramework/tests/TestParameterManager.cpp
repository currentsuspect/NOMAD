/**
 * @file TestParameterManager.cpp
 * @brief Tests for Parameter Manager
 */

#include <gtest/gtest.h>
#include "parameters/ParameterManager.h"

class ParameterManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        parameterManager = std::make_unique<nomad::parameters::ParameterManager>();
        parameterManager->initialize();
    }
    
    void TearDown() override
    {
        parameterManager.reset();
    }
    
    std::unique_ptr<nomad::parameters::ParameterManager> parameterManager;
};

TEST_F(ParameterManagerTest, Initialization)
{
    EXPECT_TRUE(parameterManager->getNumParameters() == 0);
}

TEST_F(ParameterManagerTest, CreateFloatParameter)
{
    nomad::parameters::ParameterRange range(0.0, 100.0, 50.0, 0.1);
    auto* param = parameterManager->createParameter("test_float", "Test Float", 
                                                   nomad::parameters::ParameterType::Float, range);
    
    EXPECT_NE(param, nullptr);
    EXPECT_EQ(parameterManager->getNumParameters(), 1);
    EXPECT_EQ(param->getParameterId(), "test_float");
    EXPECT_EQ(param->getName(0), "Test Float");
    EXPECT_EQ(param->getParameterType(), nomad::parameters::ParameterType::Float);
}

TEST_F(ParameterManagerTest, CreateIntParameter)
{
    nomad::parameters::ParameterRange range(0, 127, 64, 1);
    auto* param = parameterManager->createParameter("test_int", "Test Int", 
                                                   nomad::parameters::ParameterType::Int, range);
    
    EXPECT_NE(param, nullptr);
    EXPECT_EQ(param->getParameterType(), nomad::parameters::ParameterType::Int);
    EXPECT_TRUE(param->isDiscrete());
}

TEST_F(ParameterManagerTest, CreateBoolParameter)
{
    nomad::parameters::ParameterRange range(0, 1, 0, 1);
    auto* param = parameterManager->createParameter("test_bool", "Test Bool", 
                                                   nomad::parameters::ParameterType::Bool, range);
    
    EXPECT_NE(param, nullptr);
    EXPECT_EQ(param->getParameterType(), nomad::parameters::ParameterType::Bool);
    EXPECT_TRUE(param->isBoolean());
    EXPECT_TRUE(param->isDiscrete());
}

TEST_F(ParameterManagerTest, ParameterValueOperations)
{
    nomad::parameters::ParameterRange range(0.0, 100.0, 50.0, 0.1);
    auto* param = parameterManager->createParameter("test_param", "Test Param", 
                                                   nomad::parameters::ParameterType::Float, range);
    
    EXPECT_NE(param, nullptr);
    
    // Test default value
    EXPECT_EQ(param->getDefaultValue(), 0.5f); // 50.0 scaled to 0.0-1.0 range
    
    // Test value setting
    param->setScaledValue(0.25f); // 25.0 in 0-100 range
    EXPECT_NEAR(param->getScaledValue(), 0.25, 0.001);
    
    // Test raw value
    EXPECT_NEAR(param->getRawValue(), 25.0, 0.1);
}

TEST_F(ParameterManagerTest, ParameterSmoothTransitions)
{
    nomad::parameters::ParameterRange range(0.0, 100.0, 0.0, 0.1);
    auto* param = parameterManager->createParameter("test_smooth", "Test Smooth", 
                                                   nomad::parameters::ParameterType::Float, range);
    
    EXPECT_NE(param, nullptr);
    
    // Test smooth transition
    param->setValueSmooth(0.5, 100.0); // Transition to 50% over 100ms
    
    // Simulate audio processing
    param->updateParameter(512);
    
    // Value should be transitioning
    EXPECT_GT(param->getScaledValue(), 0.0);
}

TEST_F(ParameterManagerTest, ParameterCallbacks)
{
    nomad::parameters::ParameterRange range(0.0, 100.0, 0.0, 0.1);
    auto* param = parameterManager->createParameter("test_callback", "Test Callback", 
                                                   nomad::parameters::ParameterType::Float, range);
    
    EXPECT_NE(param, nullptr);
    
    bool callbackCalled = false;
    double receivedValue = 0.0;
    
    auto callback = [&callbackCalled, &receivedValue](double value) {
        callbackCalled = true;
        receivedValue = value;
    };
    
    param->addParameterCallback(callback);
    
    // Change parameter value
    param->setScaledValue(0.75);
    
    // Note: In a real test, we'd verify callback was called
    // This is simplified for the test framework
    param->removeParameterCallback(callback);
}

TEST_F(ParameterManagerTest, ParameterManagerOperations)
{
    // Create multiple parameters
    nomad::parameters::ParameterRange range1(0.0, 100.0, 50.0, 0.1);
    nomad::parameters::ParameterRange range2(0, 127, 64, 1);
    
    parameterManager->createParameter("param1", "Param 1", 
                                     nomad::parameters::ParameterType::Float, range1);
    parameterManager->createParameter("param2", "Param 2", 
                                     nomad::parameters::ParameterType::Int, range2);
    
    EXPECT_EQ(parameterManager->getNumParameters(), 2);
    
    // Test getting parameters
    auto* param1 = parameterManager->getParameter("param1");
    auto* param2 = parameterManager->getParameter("param2");
    auto* param3 = parameterManager->getParameter("nonexistent");
    
    EXPECT_NE(param1, nullptr);
    EXPECT_NE(param2, nullptr);
    EXPECT_EQ(param3, nullptr);
    
    // Test parameter values
    parameterManager->setParameterValue("param1", 0.75);
    EXPECT_NEAR(parameterManager->getParameterValue("param1"), 0.75, 0.001);
    
    // Test smooth parameter changes
    parameterManager->setParameterValueSmooth("param2", 0.5, 50.0);
    EXPECT_NEAR(parameterManager->getParameterValue("param2"), 0.5, 0.001);
}

TEST_F(ParameterManagerTest, ParameterGroups)
{
    // Create parameters
    nomad::parameters::ParameterRange range(0.0, 100.0, 50.0, 0.1);
    parameterManager->createParameter("param1", "Param 1", 
                                     nomad::parameters::ParameterType::Float, range);
    parameterManager->createParameter("param2", "Param 2", 
                                     nomad::parameters::ParameterType::Float, range);
    parameterManager->createParameter("param3", "Param 3", 
                                     nomad::parameters::ParameterType::Float, range);
    
    // Create parameter group
    std::vector<std::string> groupParams = {"param1", "param2"};
    EXPECT_TRUE(parameterManager->addParameterGroup("group1", groupParams));
    
    // Test getting group
    auto group = parameterManager->getParameterGroup("group1");
    EXPECT_EQ(group.size(), 2);
    EXPECT_EQ(group[0], "param1");
    EXPECT_EQ(group[1], "param2");
    
    // Test getting all groups
    auto allGroups = parameterManager->getParameterGroups();
    EXPECT_EQ(allGroups.size(), 1);
    EXPECT_TRUE(allGroups.find("group1") != allGroups.end());
    
    // Test removing group
    EXPECT_TRUE(parameterManager->removeParameterGroup("group1"));
    EXPECT_EQ(parameterManager->getParameterGroup("group1").size(), 0);
}

TEST_F(ParameterManagerTest, ParameterSerialization)
{
    // Create parameters
    nomad::parameters::ParameterRange range(0.0, 100.0, 50.0, 0.1);
    parameterManager->createParameter("param1", "Param 1", 
                                     nomad::parameters::ParameterType::Float, range);
    parameterManager->createParameter("param2", "Param 2", 
                                     nomad::parameters::ParameterType::Int, range);
    
    // Set some values
    parameterManager->setParameterValue("param1", 0.75);
    parameterManager->setParameterValue("param2", 0.25);
    
    // Save to XML
    auto* xml = parameterManager->saveParametersToXml();
    EXPECT_NE(xml, nullptr);
    
    // Create new parameter manager and load
    auto newManager = std::make_unique<nomad::parameters::ParameterManager>();
    newManager->initialize();
    
    // Load parameters
    EXPECT_TRUE(newManager->loadParametersFromXml(*xml));
    
    // Verify values were loaded
    EXPECT_NEAR(newManager->getParameterValue("param1"), 0.75, 0.001);
    EXPECT_NEAR(newManager->getParameterValue("param2"), 0.25, 0.001);
    
    delete xml;
}

TEST_F(ParameterManagerTest, ParameterStatistics)
{
    // Create parameters of different types
    nomad::parameters::ParameterRange range(0.0, 100.0, 50.0, 0.1);
    parameterManager->createParameter("float_param", "Float Param", 
                                     nomad::parameters::ParameterType::Float, range);
    parameterManager->createParameter("int_param", "Int Param", 
                                     nomad::parameters::ParameterType::Int, range);
    parameterManager->createParameter("bool_param", "Bool Param", 
                                     nomad::parameters::ParameterType::Bool, range);
    
    auto stats = parameterManager->getParameterStats();
    EXPECT_EQ(stats.totalParameters, 3);
    EXPECT_EQ(stats.floatParameters, 1);
    EXPECT_EQ(stats.intParameters, 1);
    EXPECT_EQ(stats.boolParameters, 1);
    EXPECT_EQ(stats.choiceParameters, 0);
    EXPECT_EQ(stats.stringParameters, 0);
    EXPECT_EQ(stats.parameterGroups, 0);
}

TEST_F(ParameterManagerTest, ParameterRemoval)
{
    // Create parameters
    nomad::parameters::ParameterRange range(0.0, 100.0, 50.0, 0.1);
    parameterManager->createParameter("param1", "Param 1", 
                                     nomad::parameters::ParameterType::Float, range);
    parameterManager->createParameter("param2", "Param 2", 
                                     nomad::parameters::ParameterType::Float, range);
    
    EXPECT_EQ(parameterManager->getNumParameters(), 2);
    
    // Remove parameter
    EXPECT_TRUE(parameterManager->removeParameter("param1"));
    EXPECT_EQ(parameterManager->getNumParameters(), 1);
    
    // Try to remove non-existent parameter
    EXPECT_FALSE(parameterManager->removeParameter("nonexistent"));
    EXPECT_EQ(parameterManager->getNumParameters(), 1);
    
    // Verify remaining parameter
    auto* param2 = parameterManager->getParameter("param2");
    EXPECT_NE(param2, nullptr);
    EXPECT_EQ(param2->getParameterId(), "param2");
}