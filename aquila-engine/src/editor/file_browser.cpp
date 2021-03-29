#include "editor/file_browser.hpp"

#include <cassert>
#include <iostream>
#include <vector>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace fs = std::filesystem;

namespace aq {    

    fs::path get_last_section(fs::path path) {
        assert(!path.empty());
        return *(--path.end());
    }

    struct FileBrowserData {
        fs::path current_folder;
        fs::path selected; // Relative to `current_folder`
        std::string file_path;
        FileBrowser::Status status = FileBrowser::Status::Selecting;
        bool show_hidden = false;

        int selected_index = -1;

        void update_file_path() {
            fs::path fp = current_folder;
            fp += fs::path::preferred_separator;
            fp += selected;
            file_path = fp.lexically_normal().c_str();
        }
    };

    FileBrowser::FileBrowser(std::string path) {
        data = new FileBrowserData{path};
        data->update_file_path();
        ImGui::OpenPopup("File Browser");
    }

    FileBrowser::~FileBrowser() { delete data; }

    bool FileBrowser::draw() {
        if (data->status == Status::Selecting && ImGui::BeginPopupModal("File Browser")) {
            if (ImGui::InputText("##FilePath", &data->file_path)) {
                bool is_valid = fs::directory_entry(data->file_path).exists();
                if (is_valid) {
                    fs::path file_path(data->file_path);
                    file_path = fs::absolute(file_path.lexically_normal());
                    data->file_path = file_path;

                    data->selected = file_path.filename();
                    data->current_folder = file_path.remove_filename();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Back")) {
                data->current_folder = data->current_folder.parent_path();
                data->selected.clear();
                data->selected_index = -1;
                data->update_file_path();
            }

            if (ImGui::Button("Confirm")) {
                data->status = Status::Confirmed;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
                data->status = Status::Canceled;
                data->selected = "";
                data->file_path = "";
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            ImGui::Checkbox("Show hidden", &data->show_hidden);

            int entry_index = 0;
            ImGui::BeginChild("File Selector", {0,0}, true);
            for (auto& entry : fs::directory_iterator(data->current_folder)) {
                bool hide = !data->show_hidden && get_last_section(entry.path()).c_str()[0] == '.';
                if (!hide && ImGui::Selectable(get_last_section(entry.path()).c_str(), entry_index == data->selected_index, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_DontClosePopups)) {
                    data->selected.clear();
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        if (entry.is_directory()) {
                            data->selected_index = -1;
                            data->current_folder = entry.path();
                        } else if (entry.is_regular_file()) {
                            data->status = Status::Confirmed;
                            data->selected = entry.path().filename();
                            ImGui::CloseCurrentPopup();
                        }
                    } else {
                        data->selected_index = entry_index;
                        data->selected = get_last_section(entry.path());
                    }
                    data->update_file_path();
                }
                ++entry_index;
            }
            ImGui::EndChild();            

            ImGui::EndPopup();
            return true;
        }
        return false;
    }

    FileBrowser::Status FileBrowser::get_status() {
        return data->status;
    }

    std::string FileBrowser::path() {
        return data->file_path;
    }

    std::filesystem::path FileBrowser::current_directory() {
        fs::path current_dir = data->current_folder;
        current_dir += fs::path::preferred_separator;
        return current_dir;
    }

    std::filesystem::path FileBrowser::current_selection() {
        return data->selected;
    }

}
