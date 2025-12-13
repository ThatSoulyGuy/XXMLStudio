cmake_minimum_required(VERSION 3.16)

project(XXMLStudio VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 with all required components
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Concurrent
    Svg
)

# =============================================================================
# Source Files organized by module
# =============================================================================

set(CORE_SOURCES
    src/core/Application.cpp
    src/core/Application.h
    src/core/Settings.cpp
    src/core/Settings.h
    src/core/IconUtils.cpp
    src/core/IconUtils.h
)

set(UI_SOURCES
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
)

set(EDITOR_SOURCES
    src/editor/CodeEditor.cpp
    src/editor/CodeEditor.h
    src/editor/XXMLSyntaxHighlighter.cpp
    src/editor/XXMLSyntaxHighlighter.h
    src/editor/EditorTabWidget.cpp
    src/editor/EditorTabWidget.h
    src/editor/BookmarkManager.cpp
    src/editor/BookmarkManager.h
    src/editor/CompletionWidget.cpp
    src/editor/CompletionWidget.h
)

set(PANEL_SOURCES
    src/panels/ProjectExplorer.cpp
    src/panels/ProjectExplorer.h
    src/panels/ProblemsPanel.cpp
    src/panels/ProblemsPanel.h
    src/panels/BuildOutputPanel.cpp
    src/panels/BuildOutputPanel.h
    src/panels/TerminalPanel.cpp
    src/panels/TerminalPanel.h
    src/panels/OutlinePanel.cpp
    src/panels/OutlinePanel.h
    src/panels/GitChangesPanel.cpp
    src/panels/GitChangesPanel.h
    src/panels/GitHistoryPanel.cpp
    src/panels/GitHistoryPanel.h
    src/panels/GitFileDecorator.cpp
    src/panels/GitFileDecorator.h
)

set(GIT_SOURCES
    src/git/GitTypes.h
    src/git/GitManager.cpp
    src/git/GitManager.h
    src/git/GitStatusModel.cpp
    src/git/GitStatusModel.h
)

set(WIDGET_SOURCES
    src/widgets/GitBranchWidget.cpp
    src/widgets/GitBranchWidget.h
    src/widgets/GitStatusIndicator.cpp
    src/widgets/GitStatusIndicator.h
)

set(LSP_SOURCES
    src/lsp/LSPClient.cpp
    src/lsp/LSPClient.h
    src/lsp/JsonRpcClient.cpp
    src/lsp/JsonRpcClient.h
    src/lsp/LSPProtocol.h
    src/lsp/DocumentSyncManager.cpp
    src/lsp/DocumentSyncManager.h
)

set(PROJECT_SOURCES
    src/project/Project.cpp
    src/project/Project.h
    src/project/ProjectManager.cpp
    src/project/ProjectManager.h
    src/project/DependencyManager.cpp
    src/project/DependencyManager.h
    src/project/GitFetcher.cpp
    src/project/GitFetcher.h
    src/project/ProjectFileParser.cpp
    src/project/ProjectFileParser.h
    src/project/LibraryProcessor.cpp
    src/project/LibraryProcessor.h
    src/project/Solution.cpp
    src/project/Solution.h
)

set(BUILD_SOURCES
    src/build/BuildManager.cpp
    src/build/BuildManager.h
    src/build/CompilerInvoker.cpp
    src/build/CompilerInvoker.h
    src/build/OutputParser.cpp
    src/build/OutputParser.h
    src/build/ProcessRunner.cpp
    src/build/ProcessRunner.h
    src/build/ToolchainLocator.cpp
    src/build/ToolchainLocator.h
)

set(DIALOG_SOURCES
    src/dialogs/FindReplaceDialog.cpp
    src/dialogs/FindReplaceDialog.h
    src/dialogs/GoToLineDialog.cpp
    src/dialogs/GoToLineDialog.h
    src/dialogs/SettingsDialog.cpp
    src/dialogs/SettingsDialog.h
    src/dialogs/NewProjectDialog.cpp
    src/dialogs/NewProjectDialog.h
    src/dialogs/ResumeProjectDialog.cpp
    src/dialogs/ResumeProjectDialog.h
    src/dialogs/DependencyDialog.cpp
    src/dialogs/DependencyDialog.h
    src/dialogs/AddDependencyDialog.cpp
    src/dialogs/AddDependencyDialog.h
    src/dialogs/SetUpstreamDialog.cpp
    src/dialogs/SetUpstreamDialog.h
)

# All sources combined
set(ALL_SOURCES
    main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${EDITOR_SOURCES}
    ${LSP_SOURCES}
    ${PANEL_SOURCES}
    ${PROJECT_SOURCES}
    ${BUILD_SOURCES}
    ${DIALOG_SOURCES}
    ${GIT_SOURCES}
    ${WIDGET_SOURCES}
    resources/resources.qrc
)

# =============================================================================
# Executable
# =============================================================================

qt_add_executable(XXMLStudio
    MANUAL_FINALIZATION
    ${ALL_SOURCES}
)

# Include directories
target_include_directories(XXMLStudio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)

# Link Qt libraries
target_link_libraries(XXMLStudio PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
    Qt6::Svg
)

# =============================================================================
# Platform-specific settings
# =============================================================================

set_target_properties(XXMLStudio PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# =============================================================================
# Installation
# =============================================================================

include(GNUInstallDirs)
install(TARGETS XXMLStudio
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# =============================================================================
# Post-build: Copy bundled toolchain
# =============================================================================

# Copy bundled toolchain to build directory for development
if(EXISTS "${CMAKE_SOURCE_DIR}/toolchain")
    add_custom_command(TARGET XXMLStudio POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/toolchain"
            "$<TARGET_FILE_DIR:XXMLStudio>/toolchain"
        COMMENT "Copying bundled XXML toolchain to build directory"
    )
endif()

qt_finalize_executable(XXMLStudio)
