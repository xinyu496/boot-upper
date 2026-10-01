include("C:/Users/Administrator/Desktop/boot_upper/build/verify/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/boot_upper-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase;qtserialport")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "C:/Users/Administrator/Desktop/boot_upper/build/verify/boot_upper.exe"
    GENERATE_QT_CONF
)
