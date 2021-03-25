#ifndef EDITOR_FILE_BROWSER_HPP
#define EDITOR_FILE_BROWSER_HPP

#include <string>
#include <optional>
#include <filesystem>

namespace aq {

    class FileBrowser {
    public:
        FileBrowser(std::string path=AQUILA_ENGINE_PATH);
        ~FileBrowser();

        // Returns false once the file browser has been closed
        bool draw();

        enum class Status {
            Selecting, // User is still browsing files
            Confirmed, // User has confirmed the file/folder they want
            Canceled
        };
        Status get_status();

        // Returns the selected path
        // Can called after `draw` returns false (user is finished with selection).
        std::string path();

        // Returns the directory the user is in (not necessarily the selected directory)
        // Will return the last selection after `draw` returns false.
        std::filesystem::path current_directory();

        // Returns the user's current selection (excluding the current directory)
        // Will return the last selection after `draw` returns false.
        std::filesystem::path current_selection();

    private:
        struct FileBrowserData* data;
    };

}

#endif