#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <sstream>

#include <locale>
#include <codecvt>

#include <iostream>
#include <unicode/unistr.h>
#include <stb/stb_image.h>

#pragma execution_character_set("utf-8")

const int WIDTH = 580;
const int HEIGHT = 780;


using json = nlohmann::json;
bool showInputFrame = false;
char newFirstName[256] = "";
char newLastName[256] = "";
char newPhoneNumber[256] = "";
char newBirthDate[256] = "";
char deleteConactChar[256] = "";

bool ContactComparator(const json& a, const json& b) {
    return a["first_name"].get<std::string>() < b["first_name"].get<std::string>();
}

void LoadContacts(json& contacts) {
    std::ifstream inputFile("contacts.json");
    if (inputFile) {
        inputFile >> contacts;
        inputFile.close();
    }
}

void RemoveContact(json& contacts, std::string& number) {
    std::ifstream inputFile("contacts.json");
    if (inputFile) {
        inputFile >> contacts;
        inputFile.close();
    }
}


void SaveContacts(const json& contacts) {
    std::ofstream outputFile("contacts.json");
    if (outputFile) {
        outputFile << contacts.dump(4);
        outputFile.close();
    }
}


// поиск по фамилии(с переводом из заглвных с строчные)
bool CaseInsensitiveStringSearch(const std::string& haystack, const std::string& needle) {
    return std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
        [](char ch1, char ch2) {
            return std::tolower(ch1) == std::tolower(ch2);
        }) != haystack.end();
}

bool PhoneNumberExists(const json& contacts, const std::string& phoneNumber) {
    if (contacts.find("contacts") != contacts.end()) {
        for (const auto& contact : contacts["contacts"]) {
            const std::string existingPhoneNumber = contact["phone_number"];
            if (existingPhoneNumber == phoneNumber) {
                return true;
            }
        }
    }

    return false;
}

std::vector<std::string> GetDays() {
    std::vector<std::string> days;
    for (int i = 1; i <= 31; ++i) {
        days.push_back(std::to_string(i));
    }
    return days;
}

std::vector<std::string> GetMonths() {
    return { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12" };
}

std::vector<std::string> GetYears() {
    std::vector<std::string> years;
    for (int i = 1900; i <= 2023; ++i) {
        years.push_back(std::to_string(i));
    }
    return years;
}


std::vector<std::string> GetDays(int selectedMonth, int selectedYear) {
    std::vector<std::string> days;
    int numDaysInMonth;


    if (selectedMonth == 1) {

        if (selectedYear % 4 == 0 && (selectedYear % 100 != 0 || selectedYear % 400 == 0)) {
            numDaysInMonth = 29;
        }
        else {
            numDaysInMonth = 28;
        }
    }
    else {

        int daysInMonths[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        numDaysInMonth = daysInMonths[selectedMonth];
    }

    for (int i = 1; i <= numDaysInMonth; ++i) {
        days.push_back(std::to_string(i));
    }
    return days;
}

bool removeContactByPhoneNumber(json& contacts, const std::string& phoneNumber) {
    if (contacts.find("contacts") != contacts.end()) {
        auto& contactsArray = contacts["contacts"];

        // Find the contact with the specified phone number
        auto it = std::remove_if(contactsArray.begin(), contactsArray.end(),
            [&phoneNumber](const auto& contact) {
                return contact["phone_number"].get<std::string>() == phoneNumber;
            });

        // Check if any contact was removed
        if (it != contactsArray.end()) {
            contactsArray.erase(it, contactsArray.end());

            // Save the updated JSON data to the file
            SaveContacts(contacts); // Assuming you have a SaveContacts function

            return true; // Contact removed successfully
        }
    }

    return false; // Contact not found
}

int InputTextCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventChar >= '0' && data->EventChar <= '9')
        return 0;  // Allow the character
    return 1;  // Block the character
}


void ParseBirthDate(const std::string& birthDate, int& day, int& month, int& year) {
    std::istringstream ss(birthDate);
    char delimiter;

    ss >> day;      // Extract day
    ss >> delimiter; // Extract the delimiter ('/')
    ss >> month;    // Extract month
    ss >> delimiter; // Extract the delimiter ('/')
    ss >> year;     // Extract year
    day--;
    month--;
    year = year - 1900;
}

struct EditContactInfo {
    char firstName[256];
    char lastName[256];
    char phoneNumber[256];
    // Add other fields as needed
};

GLuint LoadTexture(const char* path) {
    int width, height, channels;
    stbi_uc* pixels = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(pixels);
    return textureID;
}

bool getContactByPhoneNumber(const json& contacts, const std::string& phoneNumber, std::string& firstName, std::string& lastName, std::string& birthDate) {
    if (contacts.find("contacts") != contacts.end()) {
        const auto& contactsArray = contacts["contacts"];

        // Find the contact with the specified phone number
        auto it = std::find_if(contactsArray.begin(), contactsArray.end(),
            [&phoneNumber](const auto& contact) {
                return contact["phone_number"].get<std::string>() == phoneNumber;
            });

        // Check if the contact was found
        if (it != contactsArray.end()) {
            firstName = it->value("first_name", "");
            lastName = it->value("last_name", "");
            birthDate = it->value("birth_date", "");
            return true; // Contact found successfully
        }
    }

    return false; // Contact not found
}

bool phoneNumberExistEdit(const json& contacts, const std::string& newPhoneNumberInput, const std::string& tempNumberForEdit) {
    if (contacts.find("contacts") != contacts.end()) {
        for (const auto& contact : contacts["contacts"]) {
            const std::string phoneNumber = contact["phone_number"];

            
            if (phoneNumber == newPhoneNumberInput && tempNumberForEdit != newPhoneNumberInput) {
                return true;  
            }
        }
    }

    return false; 
}


std::map<std::string, EditContactInfo> editContactInfoMap;
int main() {

    if (!glfwInit()) {
        return 1;
    }


    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Contact List", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);


    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        return 1;
    }
    GLFWimage image;

    int width, height, channels;
    unsigned char* iconPixels = stbi_load("icon.png", &width, &height, &channels, 0);


  
    if (iconPixels) {
      
        GLFWimage icon;
        icon.width = width;
        icon.height = height;
        icon.pixels = iconPixels;

     
        glfwSetWindowIcon(window, 1, &icon);

     
        stbi_image_free(iconPixels);
    }


    json contacts;
    LoadContacts(contacts);

    // Сортировка фамилии в алфавитном порядке
    if (contacts.find("contacts") != contacts.end()) {
        json& contactsArray = contacts["contacts"];
        std::sort(contactsArray.begin(), contactsArray.end(), ContactComparator);
    }


    // map для того чтобы сгрупировать фамилии по их первому символу
    std::map<char, std::vector<json>> contactsByLetter;


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    (void)io;


    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // ввод
    char newFirstName[256] = "";
    char newLastName[256] = "";
    char newPhoneNumber[256] = "";
    char newBirthDate[256] = "";
    char searchLastName[256] = "";
    char deltePhoneNumber[256] = "";
    char newPhoneNumberInput[256] = "";

    int selectedDay = 1;
    int selectedMonth = 0;
    int selectedYear = 0;
    char newBirthDateDay[3] = "";
    char newBirthDateMonth[3] = "";
    char newBirthDateYear[5] = "";

    // окно с добавлением контактов
    bool showInputFrame = false;
    bool showConfirmation = false;
    bool showcConfirmationFail = false;
    bool showNoName = false;
    bool showDeleteFrame = false;
    bool showDeleteSuccess = false;
    bool showDeleteFail = false;
    bool showEditFrame = false;
    bool showEditFrame2 = false;

    std::string editFirstName;
    std::string editLastName;
    std::string editBirthDate;
    int parsedDay, parsedMonth, parsedYear;
    std::string tempNumberForEdit;


    ImGuiStyle& style = ImGui::GetStyle();

    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.FramePadding = ImVec2(8.0f, 4.0f);

    ImVec4* colors = style.Colors;
    GLuint editContactIcon = LoadTexture("edit-icon.png");
    GLuint addContactIcon = LoadTexture("add-icon.png");
    GLuint removeContactIcon = LoadTexture("remove-icon.png");
    // стили
    colors[ImGuiCol_Text] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
    colors[ImGuiCol_Border] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.24f, 0.24f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.32f, 0.32f, 0.32f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.37f, 0.8f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.0f, 0.37f, 0.8f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 0.37f, 0.8f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.0f, 0.37f, 0.8f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.58f, 1.0f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.68f, 1.0f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.0f, 0.37f, 0.8f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.0f, 0.58f, 1.0f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.68f, 1.0f, 1.0f);
    colors[ImGuiCol_Separator] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.0f, 0.37f, 0.8f, 1.0f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.0f, 0.58f, 1.0f, 1.0f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.0f, 0.37f, 0.8f, 1.0f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.0f, 0.58f, 1.0f, 1.0f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.0f, 0.68f, 1.0f, 1.0f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.0f, 0.37f, 0.8f, 0.4f);

    io.Fonts->Clear();
    io.Fonts->AddFontDefault();

    // Меняем масштаб и размер шрафта
    io.Fonts->Fonts[0]->Scale = 1.5f;
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF("C:\\Users\\mtom69\\source\\repos\\project01\\project01\\ARIAL.TTF", 18, NULL, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
    io.Fonts->Build();

    std::map<std::string, bool> editStateMap;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();


        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(WIDTH, HEIGHT));


        ImGui::Begin("Contacts", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoFocusOnAppearing);


        ImGui::SetNextItemWidth(408);
        // Поле поиска по фамилии
        ImGui::InputText("Поиск по фамилии", searchLastName, sizeof(searchLastName));
        float buttonWidth = 172.0f;
        float buttonX = ImGui::GetWindowContentRegionMax().x - buttonWidth;

        // пустота 
        ImGui::Dummy(ImVec2(buttonX, 0.0f));


        ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x);

        if (ImGui::ImageButton((ImTextureID)addContactIcon, ImVec2(32, 32))) {
            // нажата ли кнопка добавления контакта
            showInputFrame = true;
            ImGui::SetWindowFocus("Добавить контакт");
        }
        ImGui::SameLine();
    
        if (ImGui::ImageButton((ImTextureID)editContactIcon, ImVec2(32, 32))) {
            // нажата ли кнопка добавления контакта
            showEditFrame = true;
            ImGui::SetWindowFocus("Редактировать контакт");
        }
        ImGui::SameLine();
        if (ImGui::ImageButton((ImTextureID)removeContactIcon, ImVec2(32, 32))) {
            // нажата ли кнопка добавления контакта
            showDeleteFrame = true;
            ImGui::SetWindowFocus("Удалить контакт");
        }


        contactsByLetter.clear();


        // групируем контакты по первой букве из фамилии
        if (contacts.find("contacts") != contacts.end()) {
            for (const auto& contact : contacts["contacts"]) {
                const std::string firstName = contact["first_name"];
                const std::string lastName = contact["last_name"];
                const std::string phoneNumber = contact["phone_number"];
                const std::string birthDate = contact["birth_date"];

                // Фильруем контакты
                if (CaseInsensitiveStringSearch(lastName, searchLastName)) {
                    const char firstLetter = std::toupper(firstName[0]);

                    // добавить контакт в группу с одной и той же буквой
                    contactsByLetter[firstLetter].push_back(contact);
                }
            }
        }


        for (const auto& [letter, contactsForLetter] : contactsByLetter) {

            std::string letterString(1, letter);


            ImGui::Text("%s", letterString.c_str());


            for (const auto& contact : contactsForLetter) {
                const std::string firstName = contact["first_name"];
                const std::string lastName = contact["last_name"];
                const std::string phoneNumber = contact["phone_number"];
                const std::string birthDate = contact["birth_date"];

                if (ImGui::CollapsingHeader((firstName + " " + lastName).c_str())) {
                    ImGui::Text("Номер телефона:");
                    ImGui::SameLine();
                    if (ImGui::Selectable(phoneNumber.c_str())) {
                        ImGui::SetClipboardText(phoneNumber.c_str());
                    }

                    ImGui::Text("Дата рождения: %s", birthDate.c_str());

                    // Edit button
                    //if (ImGui::Button("Редактировать", ImVec2(120, 25))) {
                    //    showEditFrame = false;
                    //    // Set the fields with contact information for editing
                    //    std::memset(newFirstName, 0, sizeof(newFirstName));
                    //    std::memset(newLastName, 0, sizeof(newLastName));
                    //    std::memset(newPhoneNumber, 0, sizeof(newPhoneNumber));

                    //    strcpy_s(newFirstName, firstName.c_str());
                    //    strcpy_s(newLastName, lastName.c_str());
                    //    strcpy_s(newPhoneNumber, phoneNumber.c_str());
                    //    // Parse birthDate to extract day, month, and year
                    //    // (you need to implement this parsing function)
                    //    // ...
                    //    // Set selectedDay, selectedMonth, and selectedYear based on the parsed values
                    //    ImGui::SetWindowFocus("Edit Contact");
                    //    // Show the input frame for editing
                    //    showEditFrame = true;
                    //}
                }
            }
        }


        ImGui::End();


        if (showInputFrame) {
            ImGui::SetNextWindowPos(ImVec2(285, 300), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(370, 290));
            /*ImGui::SetNextWindowFocus();*/

            ImGui::Begin("Добавить контакт", &showInputFrame, ImGuiWindowFlags_NoResize);


            ImGui::Text("Имя");
            ImGui::SetNextItemWidth(353);
            ImGui::InputText("##FirstName", newFirstName, sizeof(newFirstName));


            ImGui::Text("Фамилия");
            ImGui::SetNextItemWidth(353);
            ImGui::InputText("##LastName", newLastName, sizeof(newLastName));


            ImGui::Text("Номер телефона");
            ImGui::SetNextItemWidth(353);
            ImGui::InputText("##PhoneNumber", newPhoneNumber, sizeof(newPhoneNumber), ImGuiInputTextFlags_CallbackCharFilter, InputTextCallback);


            ImGui::Text("Дата рождения");

            std::vector<std::string> days = GetDays();
            std::vector<std::string> months = GetMonths();
            std::vector<std::string> years = GetYears();
            ImGui::SetNextItemWidth(65);
            ImGui::Combo("День", &selectedDay, [](void* data, int idx, const char** out_text) {
                *out_text = static_cast<std::vector<std::string>*>(data)->at(idx).c_str();
            return true;
                }, static_cast<void*>(&days), static_cast<int>(days.size()));
            ImGui::SameLine();
            ImGui::SetNextItemWidth(65);
            ImGui::Combo("Месяц", &selectedMonth, [](void* data, int idx, const char** out_text) {
                *out_text = static_cast<std::vector<std::string>*>(data)->at(idx).c_str();
            return true;
                }, static_cast<void*>(&months), static_cast<int>(months.size()));
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            ImGui::Combo("Год", &selectedYear, [](void* data, int idx, const char** out_text) {
                *out_text = static_cast<std::vector<std::string>*>(data)->at(idx).c_str();
            return true;
                }, static_cast<void*>(&years), static_cast<int>(years.size()));

            ImGui::Spacing();


            if (ImGui::Button("Добавить", ImVec2(353, 35))) {
                if (strlen(newFirstName) == 0 || strlen(newLastName) == 0 || strlen(newPhoneNumber) == 0) {
                    showInputFrame = false;
                    showNoName = true;
                }
                else if (PhoneNumberExists(contacts, newPhoneNumber)) {
                    showcConfirmationFail = true;
                    showInputFrame = false;
                }
                else {
                    json newContact;
                    newContact["first_name"] = newFirstName;
                    newContact["last_name"] = newLastName;
                    newContact["phone_number"] = newPhoneNumber;
                    std::string birthDate = std::to_string(selectedDay + 1) + "/" + std::to_string(selectedMonth + 1) + "/" + std::to_string(selectedYear + 1900);
                    newContact["birth_date"] = birthDate;

                    if (contacts.find("contacts") != contacts.end()) {
                        contacts["contacts"].push_back(newContact);
                    }
                    else {
                        contacts["contacts"] = { newContact };
                    }

                    //  добавляем в json
                    SaveContacts(contacts);

                    // очиста полей
                    std::memset(newFirstName, 0, sizeof(newFirstName));
                    std::memset(newLastName, 0, sizeof(newLastName));
                    std::memset(newPhoneNumber, 0, sizeof(newPhoneNumber));
                    std::memset(newBirthDate, 0, sizeof(newBirthDate));


                    showInputFrame = false;
                    showConfirmation = true;
                }

            }

            ImGui::End();
        }

        if (showEditFrame) {
            ImGui::SetNextWindowPos(ImVec2(285, 300), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(370, 134));
            /*ImGui::SetNextWindowFocus();*/

            ImGui::Begin("Редактировать контакт 1", &showEditFrame, ImGuiWindowFlags_NoResize);






            ImGui::Text("Номер телефона");
            ImGui::SetNextItemWidth(353);
            ImGui::InputText("##PhoneNumber", newPhoneNumberInput, sizeof(newPhoneNumberInput), ImGuiInputTextFlags_CallbackCharFilter, InputTextCallback);

            ImGui::Spacing();

            if (ImGui::Button("Редактировать", ImVec2(353, 35))) {
                std::string editContactPhone(newPhoneNumberInput);
                tempNumberForEdit = editContactPhone;
                
                std::string editFirstName;
                std::string editLastName;
                std::string editBirthDate;

                if (getContactByPhoneNumber(contacts, editContactPhone, editFirstName, editLastName, editBirthDate)) {
                    strcpy_s(newFirstName, editFirstName.c_str());
                    strcpy_s(newLastName, editLastName.c_str());
                    strcpy_s(newBirthDate, editBirthDate.c_str());
                    ParseBirthDate(editBirthDate, parsedDay, parsedMonth, parsedYear);
                    showEditFrame = false;
                    showEditFrame2 = true;
                   
                    std::cout << parsedYear << " Day " << parsedDay << " Month " << parsedMonth << std::endl;
                    ImGui::SetWindowFocus("Редактировать контакт 2");
                }

           
                
              

            }


            ImGui::End();
        }
        if (showEditFrame2) {
            std::vector<std::string> days = GetDays();
            std::vector<std::string> months = GetMonths();
            std::vector<std::string> years = GetYears();
            ImGui::SetNextWindowPos(ImVec2(285, 300), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(370, 290));
            ImGui::Begin("Редактировать контакт 2", &showEditFrame2, ImGuiWindowFlags_NoResize);
            ImGui::Text("Имя");
            ImGui::SetNextItemWidth(353);
            ImGui::InputText("##EditFirstName", newFirstName, sizeof(newFirstName));

            ImGui::Text("Фамилия");
            ImGui::SetNextItemWidth(353);
            ImGui::InputText("##EditLastName", newLastName, sizeof(newLastName));

            ImGui::Text("Номер телефона");
            ImGui::SetNextItemWidth(353);
            ImGui::InputText("##PhoneNumber", newPhoneNumberInput, sizeof(newPhoneNumberInput), ImGuiInputTextFlags_CallbackCharFilter, InputTextCallback);
            ImGui::Text("Дата рождения");
            ImGui::SetNextItemWidth(65);
   
            
            ImGui::Combo("День", &parsedDay, [](void* data, int idx, const char** out_text) {
                *out_text = static_cast<std::vector<std::string>*>(data)->at(idx).c_str();
            return true;
                }, static_cast<void*>(&days), static_cast<int>(days.size()));
            ImGui::SameLine();
            ImGui::SetNextItemWidth(65);
            ImGui::Combo("Месяц", &parsedMonth, [](void* data, int idx, const char** out_text) {
                *out_text = static_cast<std::vector<std::string>*>(data)->at(idx).c_str();
            return true;
                }, static_cast<void*>(&months), static_cast<int>(months.size()));
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            ImGui::Combo("Год", &parsedYear, [](void* data, int idx, const char** out_text) {
                *out_text = static_cast<std::vector<std::string>*>(data)->at(idx).c_str();
            return true;
                }, static_cast<void*>(&years), static_cast<int>(years.size()));
            ImGui::Spacing();


            if (ImGui::Button("Сохранить", ImVec2(353, 35))) {
                std::string inputPhoneNumberEdit(newPhoneNumberInput);
                std::cout << "Name " << newFirstName << " Last name " << newLastName << std::endl;
                std::cout << "input " << inputPhoneNumberEdit << " saved " << tempNumberForEdit << std::endl;
                if (phoneNumberExistEdit(contacts, inputPhoneNumberEdit, tempNumberForEdit)) {

                }
                else {
                    std::cout << "good" << std::endl;
                    removeContactByPhoneNumber(contacts, tempNumberForEdit);
                    json newContact;
                    newContact["first_name"] = newFirstName;
                    newContact["last_name"] = newLastName;
                    newContact["phone_number"] = newPhoneNumberInput;
                    std::string birthDate = std::to_string(parsedDay + 1) + "/" + std::to_string(parsedMonth + 1) + "/" + std::to_string(parsedYear + 1900);
                    newContact["birth_date"] = birthDate;
                    if (contacts.find("contacts") != contacts.end()) {
                        contacts["contacts"].push_back(newContact);
                    }
                    else {
                        contacts["contacts"] = { newContact };
                    }

                    //  добавляем в json
                    SaveContacts(contacts);

                    // очиста полей
                    std::memset(newFirstName, 0, sizeof(newFirstName));
                    std::memset(newLastName, 0, sizeof(newLastName));
                    std::memset(newPhoneNumberInput, 0, sizeof(newPhoneNumber));
                  
                }
            }
            ImGui::End();
        }


        if (showConfirmation) {
            ImGui::SetNextWindowPos(ImVec2(285, 300), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(400, 100));
            ImGui::Begin("Подтверждение", &showConfirmation, ImGuiWindowFlags_NoResize);

            ImGui::Text("Контакт добавлен успешно!");
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("OK").x) * 0.5f);
            if (ImGui::Button("OK")) {
                showConfirmation = false;
                showInputFrame = true;
            }

            ImGui::End();
        }
        if (showcConfirmationFail) {
            ImGui::SetNextWindowPos(ImVec2(285, 300), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(336, 100));
            ImGui::Begin("Подтверждение", &showConfirmation, ImGuiWindowFlags_NoResize);
            ImGui::Text("Контакт с данным номером уже существет");
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("OK").x) * 0.5f);

            if (ImGui::Button("OK")) {
                showcConfirmationFail = false;
                showInputFrame = true;
            }

            ImGui::End();
        }
        if (showNoName) {
            ImGui::SetNextWindowPos(ImVec2(285, 300), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(336, 100));
            ImGui::Begin("Подтверждение", &showNoName, ImGuiWindowFlags_NoResize);
            ImGui::Text("Имя, Фамилия или телефое не введенны");
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("OK").x) * 0.5f);

            if (ImGui::Button("OK")) {
                showNoName = false;
                showInputFrame = true;
            }

            ImGui::End();
        }


        if (showDeleteFrame) {

            ImGui::SetNextWindowPos(ImVec2(285, 300), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(370, 134));
            /*ImGui::SetNextWindowFocus();*/

            ImGui::Begin("Удалить контакт", &showDeleteFrame, ImGuiWindowFlags_NoResize);






            ImGui::Text("Номер телефона");
            ImGui::SetNextItemWidth(353);
            ImGui::InputText("##PhoneNumber", newPhoneNumberInput, sizeof(newPhoneNumberInput), ImGuiInputTextFlags_CallbackCharFilter, InputTextCallback);

            ImGui::Spacing();

            if (ImGui::Button("Удалить", ImVec2(353, 35))) {
                std::string deleteContactPhone(newPhoneNumberInput); // Assign the value of newPhoneNumber
                // newPhoneNumber; // Remove this line, as it doesn't have any effect

                if (removeContactByPhoneNumber(contacts, deleteContactPhone)) {
                    showDeleteFrame = false;
                    showDeleteSuccess = true;
                }
                else {
                    showDeleteFail = true;
                    showDeleteFrame = false;
                }
            }


            ImGui::End();
        }

        if (showDeleteSuccess) {
            ImGui::SetNextWindowPos(ImVec2(285, 300), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(336, 100));
            ImGui::Begin("Подтверждение", &showDeleteSuccess, ImGuiWindowFlags_NoResize);
            ImGui::Text("Контакт удален");
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("OK").x) * 0.5f);
            ImGui::SetNextItemWidth(353);
            std::memset(newPhoneNumberInput, 0, sizeof(newPhoneNumberInput));
            if (ImGui::Button("OK")) {
                showDeleteSuccess = false;
            }


            ImGui::End();
        }

        if (showDeleteFail) {
            ImGui::SetNextWindowPos(ImVec2(285, 300), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(336, 100));
            ImGui::Begin("Подтверждение", &showDeleteFail, ImGuiWindowFlags_NoResize);
            ImGui::Text("Контакт не найден");
            ImGui::SetNextItemWidth(353);
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("OK").x) * 0.5f);

            if (ImGui::Button("OK")) {
                showDeleteFail = false;
            }

            ImGui::End();
        }

        if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
            showInputFrame = false;
            std::cout << "fdf";
        }

        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }


    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}