cmake_minimum_required(VERSION 3.16)
project(sozch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

add_library(sozch_model STATIC
    src/model/XmlUtil.cpp
    src/model/Entity.cpp
    src/model/NamedEntity.cpp
    src/model/Dimensions.cpp
    src/model/StockItem.cpp
    src/model/SparePartType.cpp
    src/model/PartGroup.cpp
    src/model/PartCatalog.cpp
    src/model/Warehouse.cpp
    src/model/Product.cpp
    src/model/SupplySystem.cpp
)
target_include_directories(sozch_model PUBLIC src/model)
target_link_libraries(sozch_model PUBLIC Qt5::Core)

add_executable(test_model tests/test_model.cpp)
target_link_libraries(test_model PRIVATE sozch_model)
target_compile_definitions(test_model PRIVATE
    SAMPLE_XML="${CMAKE_SOURCE_DIR}/data/supply_system.xml")

add_executable(sozch_gui
    src/gui/main.cpp
    src/gui/MainWindow.cpp
    src/gui/MainWindow.h
    src/gui/SparePartWidget.cpp
    src/gui/SparePartWidget.h
    src/gui/ProductWidget.cpp
    src/gui/ProductWidget.h
    src/gui/WarehouseWidget.cpp
    src/gui/WarehouseWidget.h
    src/gui/AddGroupDialog.cpp
    src/gui/AddGroupDialog.h
    src/gui/AddSparePartDialog.cpp
    src/gui/AddSparePartDialog.h
    src/gui/AddProductDialog.cpp
    src/gui/AddProductDialog.h
    src/gui/AddStockItemDialog.cpp
    src/gui/AddStockItemDialog.h
)
target_include_directories(sozch_gui PRIVATE src/gui src/model)
target_link_libraries(sozch_gui PRIVATE sozch_model Qt5::Widgets)
target_compile_definitions(sozch_gui PRIVATE
    SAMPLE_XML="${CMAKE_SOURCE_DIR}/data/supply_system.xml")
