// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/sstinfo.h"

#include "sst/core/build_info.h"
#include "sst/core/component.h"
#include "sst/core/elemLoader.h"
#include "sst/core/env/envconfig.h"
#include "sst/core/env/envquery.h"
#include "sst/core/model/element_python.h"
#include "sst/core/sstpart.h"
#include "sst/core/statapi/statbase.h"
#include "sst/core/statapi/statoutput.h"
#include "sst/core/subcomponent.h"
#include "sst/core/warnmacros.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <dirent.h>
#include <dlfcn.h>
#include <getopt.h>
#include <list>
#include <string>
#include <sys/stat.h>

using namespace SST;
using namespace SST::Core;

// General Global Variables
static int                                                g_fileProcessedCount;
static std::string                                        g_searchPath;
static std::vector<SSTLibraryInfo>                        g_libInfoArray;
static SSTInfoConfig                                      g_configuration(false);
static std::map<std::string, const ElementInfoGenerator*> g_foundGenerators;

// Interactive Global Variables
#ifdef HAVE_CURSES
#include <ncurses.h>
static InteractiveWindow        g_window;
static std::vector<std::string> g_infoText;
static std::deque<std::string>  g_prevInput;
static std::vector<std::string> g_libraryNames;
static unsigned int             g_textPos;
#endif


void sst_dprintf(FILE* fp, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
void
sst_dprintf(FILE* fp, const char* fmt, ...)
{
    if ( g_configuration.doVerbose() ) {
        va_list args;
        va_start(args, fmt);
        vfprintf(fp, fmt, args);
        va_end(args);
    }
}

static void xmlComment(TiXmlNode* owner, const char* fmt...) __attribute__((format(printf, 2, 3)));
static void
xmlComment(TiXmlNode* owner, const char* fmt...)
{
    ssize_t size = 128;

retry:
    char*   buf = (char*)calloc(size, 1);
    va_list ap;
    va_start(ap, fmt);
    int res = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    if ( res > size ) {
        free(buf);
        size = res + 1;
        goto retry;
    }

    TiXmlComment* comment = new TiXmlComment(buf);
    owner->LinkEndChild(comment);
}

namespace impl {
/** Trim whitespace from strings */
inline std::string
trim(std::string s)
{
    s.erase(0, s.find_first_not_of(" \t\n\r\f\v"));
    s.erase(s.find_last_not_of(" \t\n\r\f\v") + 1);
    return s;
}
} // namespace impl

class OverallOutputter
{
public:
    void outputHumanReadable(std::ostream& os);
    void outputXML();
} g_Outputter;

struct SearchableData
{
    const std::map<std::string, std::string> componentTags = { { "description", "Description" },
        { "version", "ELI version" }, { "compiledate", "Compiled on" }, { "category", "Category" },
        { "interface", "Interface" }, { "parameters", "Parameters" }, { "ports", "Ports" },
        { "subcomponents", "SubComponent Slots" }, { "statistics", "Statistics" }, { "profile", "Profile Points" },
        { "attributes", "Attributes" } };
    // Can add more in the future
} g_searchData;

// Forward Declarations
void        initLTDL(const std::string& searchPath);
void        shutdownLTDL();
static void processSSTElementFiles();
void        outputSSTElementInfo();
void        generateXMLOutputFile();
std::string parseInput(std::string);
std::string listLibraryInfo(std::list<std::string>);
std::string findLibraryInfo(std::list<std::string>);
std::string getErrorText(std::string);
std::string getErrorText(std::string, std::list<std::string>);
void        setInfoText(std::string);


#ifndef HAVE_CURSES
int
main(int argc, char* argv[])
{
    // Parse the Command Line and get the Configuration Settings
    int status = g_configuration.parseCmdLine(argc, argv);
    if ( status ) {
        if ( status == 1 ) return 0;
        return 1;
    }

    // Process all specified libraries
    g_searchPath = g_configuration.getLibPath();
    processSSTElementFiles();

    // Run interactive mode
    if ( g_configuration.interactiveEnabled() ) {
        std::cout << "Curses library not found. Run SST-Info without the -i flag." << std::endl;
    }

    return 0;
}

#else
int
main(int argc, char* argv[])
{
    // Parse the Command Line and get the Configuration Settings
    int status = g_configuration.parseCmdLine(argc, argv);
    if ( status ) {
        if ( status == 1 ) return 0;
        return 1;
    }

    // Process all specified libraries
    g_searchPath = g_configuration.getLibPath();
    processSSTElementFiles();

    // Run interactive mode
    if ( g_configuration.interactiveEnabled() ) {
        g_window.start();
    }

    return 0;
}

std::string
convertToLower(std::string input)
{
    std::transform(input.begin(), input.end(), input.begin(), ::tolower);
    return input;
}

std::string
parseInput(std::string input)
{
    // Split into set of strings
    std::istringstream       stream(input);
    std::string              word;
    std::vector<std::string> inputWords;

    while ( stream >> word ) {
        inputWords.push_back(word);
    }

    // Parse
    std::string text;
    std::string command = convertToLower(inputWords[0]);

    // Help messages
    if ( inputWords.size() == 1 ) {
        if ( command == "help" ) {
            text =
                "\n~=== SST-INFO ===~\n"
                "This program lists documented Components, SubComponents, Events, Modules, and Partitioners within an "
                "Element Library.\n\n"
                "~=== CONTROLS ===~\n"
                "The 'Console' window contains a command-line style input box. Typed input will appear here.\n"
                "The text window can be resized, and both arrow key and mouse scrolling is enabled.\n"
                "PAGE UP/PAGE DOWN scrolls through previously entered commands.\n\n"
                "~=== COMMANDS ===~\n"
                "- Help : Displays this help message\n"
                "- List {element.subelement} : Displays element libraries and component information\n"
                "- Find {field} {search string} : Displays all components with the given search string in its field\n\n"
                "- Quit : Exits the program\n\n"
                "To see more detailed instructions, type in a command without additional parameters.\n\n";
        }
        else if ( command == "list" ) {
            text = "\n~=== LIST ===~\n"
                   "Displays specified element libraries.\n\n"
                   "~=== USAGE ===~\n"
                   "- List All : Display all available element libraries and their components/subcomponents\n"
                   "- List Libraries : Display all currently loaded element libraries\n"
                   "- List [element[.component|subcomponent]] : Display the specified element/subelement(s)\n\n"
                   "'element' - Element Library\n"
                   "'type' - Type of Component/Subcomponent\n"
                   "'component|subcomponent' - Either a Component or SubComponent defined in the Element Library\n\n"
                   "~=== EXAMPLES ===~\n"
                   "list coreTestElement\n"
                   "list sst.linear\n"
                   "list ariel miranda\n"
                   "list ariel miranda.ReverseSingleStreamGenerator\n";
        }
        else if ( command == "find" ) {
            text = "\n~=== FIND ===~\n"
                   "Search for text within component library fields.\n"
                   "Displays all loaded components with the specified text.\n\n"
                   "~=== USAGE ===~\n"
                   "- Find {field} [search term] \n\n"
                   "'field' - Component/subcomponent fields.\n"
                   "Valid field keywords (case-insensitive):\n"
                   "   - Description\n"
                   "   - Version (ELI Version)\n"
                   "   - Compiledate\n"
                   "   - Category\n"
                   "   - Interface\n"
                   "   - Parameters\n"
                   "   - Ports\n"
                   "   - Subcomponents\n"
                   "   - Statistics\n"
                   "   - Profile\n"
                   "   - Attributes\n"
                   "Search term can be multiple words, but is case-sensitive.\n\n"
                   "~=== EXAMPLES ===~\n"
                   "find Description test\n"
                   "find compiledate Oct 17\n"
                   "find CATEGORY UNCATEGORIZED\n"
                   "find parameters rng";
        }
        else if ( command == "quit" ) {
            return command;
        }
        else {
            text = getErrorText(command);
        }
    }

    // Parse commands
    else {
        // Get args
        auto                   start = std::next(inputWords.begin(), 1);
        auto                   end   = inputWords.end();
        std::list<std::string> args(start, end);

        if ( command == "list" ) {
            text = listLibraryInfo(args);
        }
        else if ( command == "find" ) {
            text = findLibraryInfo(args);

            // Clear Library Info text of highlight characters
            for ( auto& library : g_libInfoArray ) {
                library.clearHighlights();
            }
        }

        // Handle errors from getting text from library info
        if ( text == "ERR" ) {
            text = getErrorText(command, args);
        }
    }

    return text;
}

int
addLibFilter(std::string libFilter, std::string componentFilter = "")
{
    for ( auto& library : g_libInfoArray ) {
        if ( library.getLibraryName() == libFilter ) {
            library.setLibraryFilter(true);

            if ( componentFilter != "" ) {
                return library.setComponentFilter(componentFilter);
            }
            return 0;
        }
    }
    // Error - library not found
    return 1;
}

// 'List' command
std::string
listLibraryInfo(std::list<std::string> args)
{
    std::stringstream outputStream;

    // Handle special keywords
    if ( args.size() == 1 ) {
        std::string arg = convertToLower(args.front());

        if ( arg == "all" ) {
            outputStream << "\n~-Displaying All Libraries-~\n";
            for ( auto& library : g_libInfoArray ) {
                library.resetFilters(true);
                library.outputText(outputStream);
            }
            return outputStream.str();
        }
        else if ( arg == "libraries" ) {
            outputStream << "\n~-All Loaded Libraries-~\n";
            for ( auto& library : g_libInfoArray ) {
                outputStream << "\n - " << library.getLibraryName();
            }
            return outputStream.str();
        }
    }

    // Reset all filters
    for ( auto& library : g_libInfoArray ) {
        library.resetFilters(false);
    }

    outputStream << "\n~-Displaying:";
    for ( std::string arg : args ) {
        outputStream << " " + arg;
        std::string library   = "";
        std::string component = "";

        // Parse library.component
        size_t split = arg.find('.');
        if ( split == std::string::npos ) {
            library = arg;
        }
        else {
            library   = arg.substr(0, split);
            component = arg.substr(split + 1);
        }

        // Check for invalid library name
        if ( addLibFilter(library, component) ) {
            return "ERR";
        }
    }
    outputStream << "-~\n";

    for ( auto& library : g_libInfoArray ) {
        library.outputText(outputStream);
    }

    return outputStream.str();
}

// 'Find' command
std::string
findLibraryInfo(std::list<std::string> args)
{
    std::stringstream outputStream;

    if ( args.size() == 1 ) {
        return "Missing search term -- See 'find' documentation to see usage.";
    }

    std::string inputTag = convertToLower(args.front());

    // Compare to tag list and convert to proper string
    auto mapIter = g_searchData.componentTags.find(inputTag);

    if ( mapIter != g_searchData.componentTags.end() ) {
        std::string tag = mapIter->second;

        args.pop_front();
        std::string searchTerm = "";
        for ( std::string arg : args ) {
            if ( arg == args.back() ) {
                searchTerm += arg;
            }
            else {
                searchTerm += arg + " ";
            }
        }

        // Search through libraries
        for ( auto& library : g_libInfoArray ) {
            library.resetFilters(false);
            library.filterSearch(outputStream, tag, searchTerm);
            library.outputText(outputStream);
        }
        return outputStream.str();
    }

    return "ERR";
}

// Finds the closest term using Levenshtein distance
std::string
getClosestTerm(std::string source, std::list<std::string> dict)
{
    std::string closest  = "\n";
    int         distance = INT_MAX;
    for ( auto& term : dict ) {
        int                           m = source.length();
        int                           n = term.length();
        std::vector<std::vector<int>> matrix(m + 1, std::vector(n + 1, 0));

        for ( int i = 0; i <= m; i++ ) {
            matrix[i][0] = i;
        }
        for ( int j = 0; j <= n; j++ ) {
            matrix[0][j] = j;
        }

        const char* s = source.c_str();
        const char* t = term.c_str();

        for ( int j = 1; j <= n; j++ ) {
            for ( int i = 1; i <= m; i++ ) {
                int subCost = 0;
                if ( s[i] != t[j] ) {
                    subCost = 1;
                }
                matrix[i][j] = std::min({ matrix[i - 1][j] + 1, matrix[i][j - 1] + 1, matrix[i - 1][j - 1] + subCost });
            }
        }

        // Get final score and set closest term
        if ( matrix[m][n] < distance ) {
            closest  = term;
            distance = matrix[m][n];
        }
    }

    return closest;
}

// Handle misspelled commands
std::string
getErrorText(std::string command)
{
    std::list<std::string> dict = { "help", "list", "find" };

    std::string term = getClosestTerm(command, dict);

    return "Invalid command '" + command + "' -- Did you mean '" + term + "'?";
}

// Handle misspelled Libraries or search terms
std::string
getErrorText(std::string command, std::list<std::string> args)
{
    if ( command == "find" ) {
        std::list<std::string> dict;
        for ( auto const& tag : g_searchData.componentTags ) {
            dict.push_back(tag.first);
        }
        std::string term = getClosestTerm(args.front(), dict);

        return "Invalid search term '" + args.front() + "'-- Did you mean '" + term + "'?";
    }
    else {
        std::string output = "Invalid Library input:\n\n";

        // Find error distances for every library entered
        for ( auto& arg : args ) {
            std::string library;
            std::string component = "";

            size_t split = arg.find('.');
            if ( split == std::string::npos ) {
                library = arg;
            }
            else {
                library   = arg.substr(0, split);
                component = arg.substr(split + 1);
            }

            // Build search dictionaries
            std::list<std::string> library_dict;
            std::list<std::string> component_dict;
            for ( auto& lib : g_libInfoArray ) {
                library_dict.push_back(lib.getLibraryName());

                if ( component != "" ) {
                    for ( auto& pair : lib.getComponentInfo() ) {
                        for ( auto& comp : pair.second ) {
                            component_dict.push_back(comp.componentName);
                        }
                    }
                }
            }

            // Find closest term for all Library entries
            std::string closest_lib = getClosestTerm(library, library_dict);
            std::string closest_comp;
            if ( component != "" ) {
                closest_comp = getClosestTerm(component, component_dict);

                if ( library != closest_lib ) {
                    if ( component != closest_comp ) {
                        output += " - `" + library + "." + component + "` --- Did you mean '" + closest_lib + "." +
                                  closest_comp + "'?\n";
                    }
                    else {
                        output += " - `" + library + "`." + component + "` --- Did you mean '" + closest_lib + "'?\n";
                    }
                }
                else {
                    if ( component != closest_comp ) {
                        output += " - " + library + ".`" + component + "` --- Did you mean '" + closest_comp + "'?\n";
                    }
                    else {
                        output += " - " + arg + "\n";
                    }
                }
            }
            else {
                if ( library != closest_lib ) {
                    output += " - `" + library + "` --- Did you mean '" + closest_lib + "'?\n";
                }
                else {
                    output += " - " + arg + "\n";
                }
            }
        }
        return output;
    }
}


void
setInfoText(std::string infoString)
{
    std::vector<std::string> stringVec;
    g_textPos = 0;

    // Splits the string into individual lines and stores them into the infoText vector
    std::stringstream infoStream(infoString);
    std::string       line;

    while ( std::getline(infoStream, line, '\n') ) {
        stringVec.push_back((line + "\n"));
    }
    g_infoText.clear(); // clears memory
    g_infoText = stringVec;
}
#endif

static void
addELI(ElemLoader& loader, const std::string& lib, bool optional)
{

    if ( g_configuration.debugEnabled() ) fprintf(stdout, "Looking for library \"%s\"\n", lib.c_str());

    std::stringstream err_sstr;
    loader.loadLibrary(lib, err_sstr);

    // Check to see if this library loaded into the new ELI
    // Database
    if ( ELI::LoadedLibraries::isLoaded(lib) ) {
        g_fileProcessedCount++;
        g_libInfoArray.emplace_back(lib);
    }
    else if ( !optional ) {
        fprintf(stderr, "**** WARNING - UNABLE TO PROCESS LIBRARY = %s\n", lib.c_str());
        fprintf(
            stderr, "**** CHECK: Has this library been registered with sst-core using the 'sst-register' utility?\n");
        fprintf(stderr, "**** CHECK: sst-info searches are case-sensitive\n");
        fprintf(stderr, "**** CHECK: Do not include the prefix or file extension when using the lib option.\n");
        fprintf(stderr, "**** EXAMPLE: 'sst-info -l PaintShop' to display model information from libPaintShop.so\n");
        if ( g_configuration.debugEnabled() ) {
            std::cerr << err_sstr.str() << std::endl;
        }
    }
    else {
        fprintf(stderr, "**** %s not Found!\n", lib.c_str());
        // regardless of debug - force error printing
        std::cerr << err_sstr.str() << std::endl;
    }
}

static void
processSSTElementFiles()
{
    std::vector<bool>        EntryProcessedArray;
    ElemLoader               loader(g_searchPath);
    std::vector<std::string> potentialLibs;
    loader.getPotentialElements(potentialLibs);

    // Which libraries should we (attempt) to process
    std::set<std::string> processLibs(g_configuration.getElementsToProcessArray());
    if ( processLibs.empty() ) {
        for ( auto& i : potentialLibs )
            processLibs.insert(i);
    }

    for ( auto l : processLibs ) {
        addELI(loader, l, g_configuration.processAllElements());
    }

    // Store info strings for interactive mode
    if ( g_configuration.interactiveEnabled() ) {
        for ( size_t x = 0; x < g_libInfoArray.size(); x++ ) {
            g_libInfoArray[x].setAllLibraryInfo();
        }
    }
    else {
        // Do we output in Human Readable form
        if ( g_configuration.getOptionBits() & CFG_OUTPUTHUMAN ) {
            outputSSTElementInfo();
        }

        // Do we output an XML File
        if ( g_configuration.getOptionBits() & CFG_OUTPUTXML ) {
            generateXMLOutputFile();
        }
    }
}

void
outputSSTElementInfo()
{
    g_Outputter.outputHumanReadable(std::cout);
}

void
generateXMLOutputFile()
{
    g_Outputter.outputXML();
}


void
OverallOutputter::outputHumanReadable(std::ostream& os)
{
    os << "PROCESSED " << g_fileProcessedCount << " .so (SST ELEMENT) FILES FOUND IN DIRECTORY(s) " << g_searchPath
       << "\n";

    // Tell the user what Elements will be displayed
    for ( auto& i : g_configuration.getFilterMap() ) {
        fprintf(stdout, "Filtering output on Element = \"%s", i.first.c_str());
        if ( !i.second.empty() ) fprintf(stdout, ".%s", i.second.c_str());
        fprintf(stdout, "\"\n");
    }

    // Now dump the Library Info
    for ( size_t x = 0; x < g_libInfoArray.size(); x++ ) {
        g_libInfoArray[x].outputHumanReadable(os, x);
    }
}

void
OverallOutputter::outputXML()
{
    unsigned int x;
    char         TimeStamp[32];
    std::time_t  now = std::time(nullptr);
    std::tm*     ptm = std::localtime(&now);

    // Create a Timestamp Format: 2015.02.15_20:20:00
    std::strftime(TimeStamp, 32, "%Y.%m.%d_%H:%M:%S", ptm);

    sst_dprintf(stdout, "\n");
    sst_dprintf(stdout, "================================================================================\n");
    sst_dprintf(stdout, "GENERATING XML FILE SSTInfo.xml as %s\n", g_configuration.XMLFilePath().c_str());
    sst_dprintf(stdout, "================================================================================\n");
    sst_dprintf(stdout, "\n");
    sst_dprintf(stdout, "\n");

    // Create the XML Document
    TiXmlDocument XMLDocument;

    // Set the Top Level Element
    TiXmlElement* XMLTopLevelElement = new TiXmlElement("SSTInfoXML");

    // Set the File Information
    TiXmlElement* XMLFileInfoElement = new TiXmlElement("FileInfo");
    XMLFileInfoElement->SetAttribute("SSTInfoVersion", PACKAGE_VERSION);
    XMLFileInfoElement->SetAttribute("FileFormat", "1.0");
    XMLFileInfoElement->SetAttribute("TimeStamp", TimeStamp);
    XMLFileInfoElement->SetAttribute("FilesProcessed", g_fileProcessedCount);
    XMLFileInfoElement->SetAttribute("SearchPath", g_searchPath.c_str());

    // Add the File Information to the Top Level Element
    XMLTopLevelElement->LinkEndChild(XMLFileInfoElement);

    // Now Generate the XML Data that represents the Library Info,
    // and add the data to the Top Level Element
    for ( x = 0; x < g_libInfoArray.size(); x++ ) {
        g_libInfoArray[x].outputXML(x, XMLTopLevelElement);
    }

    // Add the entries into the XML Document
    // XML Declaration
    TiXmlDeclaration* XMLDecl = new TiXmlDeclaration("1.0", "", "");
    XMLDocument.LinkEndChild(XMLDecl);

    // General Info on the Data
    xmlComment(&XMLDocument, "SSTInfo XML Data Generated on %s", TimeStamp);
    xmlComment(&XMLDocument, "%d .so FILES FOUND IN DIRECTORY(s) %s\n", g_fileProcessedCount, g_searchPath.c_str());

    XMLDocument.LinkEndChild(XMLTopLevelElement);

    // Save the XML Document
    XMLDocument.SaveFile(g_configuration.XMLFilePath().c_str());
}

SSTInfoConfig::SSTInfoConfig(bool suppress_print) :
    ConfigShared()
{
    using namespace std::placeholders;

    if ( !suppress_print ) enable_printing();

    m_optionBits = CFG_OUTPUTHUMAN | CFG_VERBOSE; // Enable normal output by default

    // Add the options
    DEF_SECTION_HEADING("Informational Options");
    DEF_FLAG("help", 'h', "Print help message", help_);
    DEF_FLAG("version", 'V', "Print SST release version", version_);

    DEF_SECTION_HEADING("Display Options");
    addVerboseOptions(false);
    DEF_FLAG("quiet", 'q', "Quiet/print summary only", quiet_);
    DEF_FLAG("debug", 'd', "Enable debugging messages", debugEnabled_);
    DEF_FLAG("nodisplay", 'n', "Do not display output [default: off]", no_display_);
    DEF_FLAG("interactive", 'i', "(EXPERIMENTAL) Enable interactive command line mode", interactiveEnabled_);

    DEF_SECTION_HEADING("XML Options");
    DEF_FLAG("xml", 'x', "Generate XML data [default:off]", xml_);
    DEF_ARG("outputxml", 'o', "FILE", "Filepath to XML file [default: SSTInfo.xml]", XMLFilePath_, false);

    DEF_SECTION_HEADING("Library and Path Options");
    DEF_ARG("libs", 'l', "LIBS",
        "Element libraries to process (all, <element>) [default: all]. <element> can be an element library, or it can "
        "be a single element within the library.",
        libs_, false);
    addLibraryPathOptions();

    DEF_SECTION_HEADING("Advanced Options - Environment");
    addEnvironmentOptions();

    addPositionalCallback(std::bind(&SSTInfoConfig::setPositionalArg, this, _1, _2));
}

std::string
SSTInfoConfig::getUsagePrelude()
{
    std::string prelude = "Usage: ";
    prelude.append(getRunName());
    prelude.append(" [options] [<element[.component|subcomponent]>]\n");
    return prelude;
}

void
SSTInfoConfig::outputUsage()
{
    using std::cout;
    using std::endl;
    cout << "Usage: " << m_AppName << " [<element[.component|subcomponent]>] "
         << " [options]" << endl;
    cout << "  -h, --help               Print Help Message\n";
    cout << "  -v, --version            Print SST Package Release Version\n";
    cout << "  -d, --debug              Enabled debugging messages\n";
    cout << "  -n, --nodisplay          Do not display output - default is off\n";
    cout << "  -x, --xml                Generate XML data - default is off\n";
    cout << "  -o, --outputxml=FILE     File path to XML file. Default is SSTInfo.xml\n";
    cout << "  -l, --libs=LIBS          {all, <elementname>} - Element Library(s) to process\n";
    cout << "  -q, --quiet              Quiet/print summary only\n";
    cout << endl;
}

void
SSTInfoConfig::addFilter(const std::string& name_str)
{
    std::string name(name_str);
    if ( name.size() > 3 && name.substr(0, 3) == "lib" ) name = name.substr(3);

    size_t dotLoc = name.find(".");
    if ( dotLoc == std::string::npos ) {
        m_filters.insert(std::make_pair(name, ""));
    }
    else {
        m_filters.insert(std::make_pair(std::string(name, 0, dotLoc), std::string(name, dotLoc + 1)));
    }
}

bool
doesLibHaveFilters(const std::string& libName)
{
    auto range = g_configuration.getFilterMap().equal_range(libName);
    for ( auto x = range.first; x != range.second; ++x ) {
        if ( x->second != "" ) return true;
    }
    return false;
}

bool
shouldPrintElement(const std::string& libName, const std::string& elemName)
{
    auto range = g_configuration.getFilterMap().equal_range(libName);
    if ( range.first == range.second ) return true;
    for ( auto x = range.first; x != range.second; ++x ) {
        if ( x->second == "" ) return true;
        if ( x->second == elemName ) return true;
    }
    return false;
}

void
SSTLibraryInfo::setLibraryInfo(std::string baseName, std::string componentName, std::string info)
{
    ComponentInfo                      componentInfo;
    std::map<std::string, std::string> infoMap;

    // Split string into lines and map each key:value pair
    std::stringstream infoStream(info);
    std::string       line;
    while ( std::getline(infoStream, line, '\n') ) {
        size_t split = line.find(':');

        std::string tag;
        std::string value;

        if ( split == std::string::npos ) {
            tag   = line;
            value = "";
        }
        else {
            tag   = line.substr(0, split);
            value = line.substr(split + 1);
        }

        infoMap.insert(make_pair(tag, value));
        componentInfo.stringIndexer.push_back(tag);
    }

    componentInfo.componentName = componentName;
    componentInfo.infoMap       = infoMap;

    // Add to component lists
    m_componentNames.push_back(componentName);
    m_components[baseName].push_back(componentInfo);
}

void
SSTLibraryInfo::outputText(std::stringstream& outputStream)
{
    using std::endl;
    if ( this->m_libraryFilter ) {
        outputStream << "\n================================================================================\n";
        outputStream << "ELEMENT LIBRARY: " << this->m_name << endl;

        // Loop over component types
        for ( auto& pair : this->m_components ) {
            std::string componentType = pair.first;
            outputStream << componentType << "s (" << pair.second.size() << " total)\n";

            // Loop over each component
            for ( int idx = 0; idx < int(pair.second.size()); idx++ ) {
                auto component = pair.second[idx];

                // Apply filter
                bool filtered = std::find(m_componentFilters.begin(), m_componentFilters.end(),
                                    component.componentName) != m_componentFilters.end();
                if ( (m_componentFilters.size() == 0) || filtered ) {
                    outputStream << "  " << componentType << " " << idx << ": " << component.componentName << endl;

                    // Iterate through infoMap using the string indexer
                    for ( auto key : component.stringIndexer ) {
                        std::string val = component.infoMap[key];

                        if ( val == "" ) {
                            outputStream << key << endl;
                        }
                        else {
                            outputStream << key << ": " << val << endl;
                        }
                    }
                    outputStream << endl;
                }
            }
        }
    }
}

void
SSTLibraryInfo::filterSearch(std::stringstream& outputStream, std::string tag, std::string searchTerm)
{
    std::vector<std::string> stringList = { "Parameters", "Ports", "SubComponent Slots", "Statistics", "Profile Points",
        "Attributes" };
    std::string              prefix     = "         ";
    int                      count      = 0;
    for ( auto& pair : m_components ) {
        for ( auto& component : pair.second ) {
            std::string searchString = "";
            bool        found        = false;
            std::string fullTag      = "";
            for ( auto& mapTag : component.stringIndexer ) {
                std::string        temp;
                std::istringstream stream(mapTag);
                std::getline(stream, temp, '(');
                std::string infoTag = impl::trim(temp);

                // Search for correct tag
                if ( infoTag == tag ) {
                    fullTag = mapTag;
                    if ( std::find(stringList.begin(), stringList.end(), infoTag) != stringList.end() ) {
                        found = true;
                    }
                    else {
                        searchString = component.infoMap[mapTag];
                        break;
                    }
                }
                else {
                    // Get sub-items
                    std::string info_prefix = mapTag.substr(0, 9);
                    if ( found && info_prefix == prefix ) {
                        // Set highlighting
                        size_t foundSecondIdx = component.infoMap[mapTag].find(searchTerm);
                        if ( foundSecondIdx != std::string::npos ) {
                            component.infoMap[mapTag].insert(foundSecondIdx, "`");
                            component.infoMap[mapTag].insert(foundSecondIdx + searchTerm.size() + 1, "`");
                        }

                        searchString += mapTag + ": " + component.infoMap[mapTag] + "\n";
                    }
                    else {
                        if ( found ) {
                            break;
                        }
                    }
                }
            }

            // If term is found, set Library to show and add component to filters
            size_t foundIdx = searchString.find(searchTerm);
            if ( foundIdx != std::string::npos ) {
                m_libraryFilter = true;
                m_componentFilters.push_back(component.componentName);
                count++;

                // Highlight the found text
                if ( component.infoMap[fullTag] != "" ) {
                    component.infoMap[fullTag].insert(foundIdx, "`");
                    component.infoMap[fullTag].insert(foundIdx + searchTerm.length() + 1, "`");
                }
            }
        }
    }

    outputStream << "\n~-Found " << count
                 << " components in " + m_name + " with '" + searchTerm + "' in '" + tag + "'-~\n\n";
}

void
SSTLibraryInfo::clearHighlights()
{
    for ( auto& pair : m_components ) {
        for ( auto& component : pair.second ) {
            for ( auto& info : component.infoMap ) {
                std::string* infoStr = &info.second;
                infoStr->erase(remove(infoStr->begin(), infoStr->end(), '`'), infoStr->end());
            }
        }
    }
}

template <class BaseType>
void
SSTLibraryInfo::setAllLibraryInfo()
{
    // lib is an InfoLibrary
    auto* lib = ELI::InfoDatabase::getLibrary<BaseType>(getLibraryName());
    if ( lib ) {
        // Only print if there is something of that type in the library
        if ( lib->numEntries() != 0 ) {
            // Create map keys based on type name
            std::string baseName = std::string(BaseType::ELI_baseName());

            // lib->getMap returns a map<string, BaseInfo*>.  BaseInfo is
            // actually a Base::BuilderInfo and the implementation is in
            // eli/elementinfo as BuilderInfoImpl
            for ( auto& map : lib->getMap() ) {
                std::stringstream infoStream;
                map.second->toString(infoStream);

                setLibraryInfo(baseName, map.first, infoStream.str());
            }
        }
    }
    else {
        // os << "No " << BaseType::ELI_baseName() << "s\n";
    }
}

void
SSTLibraryInfo::setAllLibraryInfo()
{

    setAllLibraryInfo<Component>();
    setAllLibraryInfo<SubComponent>();
    setAllLibraryInfo<Module>();
    setAllLibraryInfo<SST::Partition::SSTPartitioner>();
    setAllLibraryInfo<SST::Profile::ProfileTool>();
}

template <class BaseType>
void
SSTLibraryInfo::outputHumanReadable(std::ostream& os, bool printAll)
{
    // lib is an InfoLibrary
    auto* lib = ELI::InfoDatabase::getLibrary<BaseType>(getLibraryName());
    if ( lib ) {
        // Only print if there is something of that type in the library
        if ( lib->numEntries() != 0 ) {
            os << BaseType::ELI_baseName() << "s (" << lib->numEntries(true) << " total)\n";
            int idx = 0;
            // lib->getMap returns a map<string, BaseInfo*>.  BaseInfo is
            // actually a Base::BuilderInfo and the implementation is in
            // BuildInfoImpl
            for ( auto& pair : lib->getMap() ) {
                // Need to skip aliases, unless it was specifically
                // called out (this will be the case if printAll is
                // false, but shouldPrintElement() is true)
                bool is_alias = (pair.second->getAlias() == pair.first);
                bool print = (!is_alias && printAll) || (!printAll && shouldPrintElement(getLibraryName(), pair.first));

                if ( print ) {
                    os << "   " << BaseType::ELI_baseName() << " " << idx << ": " << pair.first << "\n";
                    if ( g_configuration.doVerbose() ) pair.second->toString(os);
                }
                if ( print ) ++idx;
                if ( print ) os << std::endl;
            }
        }
    }
    else {
        os << "No " << BaseType::ELI_baseName() << "s\n";
    }
}

void
SSTLibraryInfo::outputHumanReadable(std::ostream& os, int LibIndex)
{
    bool enableFullElementOutput = !doesLibHaveFilters(getLibraryName());

    os << "================================================================================\n";
    os << "ELEMENT LIBRARY " << LibIndex << " = " << getLibraryName() << " (" << getLibraryDescription() << ")"
       << "\n";

    outputHumanReadable<Component>(os, enableFullElementOutput);
    outputHumanReadable<SubComponent>(os, enableFullElementOutput);
    outputHumanReadable<Module>(os, enableFullElementOutput);
    outputHumanReadable<SST::Partition::SSTPartitioner>(os, enableFullElementOutput);
    outputHumanReadable<SST::Profile::ProfileTool>(os, enableFullElementOutput);
    outputHumanReadable<SST::SSTElementPythonModule>(os, enableFullElementOutput);
    outputHumanReadable<SST::Statistics::StatisticOutput>(os, enableFullElementOutput);
    // template param is not part of output so only need to include one
    outputHumanReadable<SST::Statistics::Statistic<uint32_t>>(os, enableFullElementOutput);
}

template <class BaseType>
void
SSTLibraryInfo::outputXML(TiXmlElement* XMLLibraryElement)
{
    auto* lib = ELI::InfoDatabase::getLibrary<BaseType>(getLibraryName());
    if ( lib ) {
        int numObjects = lib->numEntries();
        xmlComment(XMLLibraryElement, "Num %ss = %d", BaseType::ELI_baseName(), numObjects);
        int idx = 0;
        for ( auto& pair : lib->getMap() ) {
            TiXmlElement* XMLElement = new TiXmlElement(BaseType::ELI_baseName());
            XMLElement->SetAttribute("Index", idx);
            if ( pair.first != pair.second->getAlias() ) {
                pair.second->outputXML(XMLElement);
                XMLLibraryElement->LinkEndChild(XMLElement);
                idx++;
            }
        }
    }
    else {
        xmlComment(XMLLibraryElement, "No %ss", BaseType::ELI_baseName());
    }
}

void
SSTLibraryInfo::outputXML(int LibIndex, TiXmlNode* XMLParentElement)
{
    TiXmlElement* XMLLibraryElement = new TiXmlElement("Element");
    XMLLibraryElement->SetAttribute("Index", LibIndex);
    XMLLibraryElement->SetAttribute("Name", getLibraryName().c_str());
    XMLLibraryElement->SetAttribute("Description", getLibraryDescription().c_str());

    outputXML<Component>(XMLLibraryElement);
    outputXML<SubComponent>(XMLLibraryElement);
    outputXML<Module>(XMLLibraryElement);
    outputXML<SST::Partition::SSTPartitioner>(XMLLibraryElement);
    XMLParentElement->LinkEndChild(XMLLibraryElement);
}

#ifdef HAVE_CURSES
void
InteractiveWindow::start()
{
    g_textPos = 0;

    initscr();
    cbreak();
    noecho();
    draw();
    setInfoText(parseInput("help"));
    printInfo();

    // Loop for input
    getInput();
    endwin();
}

void
InteractiveWindow::getInput()
{
    std::string input        = "";
    std::string output       = "";
    std::string stashedInput = "";
    int         entryIdx     = -1;

    // Main loop for console input
    while ( true ) {
        int c = wgetch(console);

        if ( c == '\n' ) {
            if ( input != "" ) {
                g_prevInput.push_front(input);
                output = parseInput(input);

                if ( output == "quit" ) {
                    break;
                }

                setInfoText(output);
                g_window.draw();
                g_window.printInfo();
                input    = "";
                entryIdx = -1;
            }
        }
        // Resizing the window
        else if ( c == KEY_RESIZE ) {
            g_window.draw();
            g_window.printInfo();
        }
        // Handle backspaces
        else if ( c == KEY_BACKSPACE ) {
            int pos = g_window.getCursorPos();
            if ( pos > 1 ) {
                g_window.printConsole("\b \b");
                input.pop_back();
            }
        }
        // Scrolling
        else if ( c == KEY_UP ) {
            if ( g_textPos > 0 ) {
                g_textPos -= 1;
                g_window.printInfo();
            }
        }
        else if ( c == KEY_DOWN ) {
            if ( (int)g_textPos < (int)g_infoText.size() - (int)LINES + 3 ) {
                g_textPos += 1;
                g_window.printInfo();
            }
        }
        // Cycle through previous commands
        else if ( c == KEY_PPAGE ) {
            if ( entryIdx == -1 ) {
                stashedInput = input;
            }

            if ( entryIdx < int(g_prevInput.size() - 1) ) {
                entryIdx++;
                input = g_prevInput[entryIdx];

                g_window.draw();
                g_window.printInfo();
                g_window.printConsole(input.c_str());
            }
        }
        else if ( c == KEY_NPAGE ) {
            if ( entryIdx >= 0 ) {
                entryIdx--;
                if ( entryIdx == -1 ) {
                    input = stashedInput;
                }
                else {
                    input = g_prevInput[entryIdx];
                }

                g_window.draw();
                g_window.printInfo();
                g_window.printConsole(input.c_str());
            }
        }
        // Regular characters
        else if ( c <= 255 ) {
            input += c;
            std::string letter(1, c);
            g_window.printConsole(letter.c_str());
        }

        // Make sure the cursor resets to the correct place
        g_window.resetCursor(input.size() + 1);
    }
    endwin();
}

void
InteractiveWindow::draw(bool drawConsole)
{
    werase(info);
    delwin(info);
    info = newwin(LINES - 3, COLS, 0, 0);
    scrollok(info, true);
    wrefresh(info);

    if ( drawConsole ) {
        werase(console);
        delwin(console);
        console = newwin(3, COLS, LINES - 3, 0);
        scrollok(console, false);
        keypad(console, true);
        box(console, 0, 0);
        mvwprintw(console, 0, 1, " Console ");
        wmove(console, 1, 1);
        wrefresh(console);
    }
}

void
InteractiveWindow::printInfo()
{
    unsigned int posMax =
        ((int)g_infoText.size() < LINES - 3) ? g_textPos + g_infoText.size() : g_textPos + (LINES - 3);

    std::string infoString = "";
    for ( unsigned int i = g_textPos; i < posMax; i++ ) {
        infoString += g_infoText[i];
    }

    bool bold      = false;
    bool highlight = false;
    for ( auto& c : infoString ) {
        if ( c == '~' && !bold ) {
            wattron(info, A_BOLD);
            bold = true;
        }
        else if ( c == '~' && bold ) {
            wattroff(info, A_BOLD);
            bold = false;
        }
        else if ( c == '`' && !highlight ) {
            wattron(info, A_STANDOUT);
            highlight = true;
        }
        else if ( c == '`' && highlight ) {
            wattroff(info, A_STANDOUT);
            highlight = false;
        }
        else {
            wprintw(info, "%c", c);
        }
    }
    wrefresh(info);
    wrefresh(console); // moves the cursor back into the console window
    wmove(console, 1, 1);
}
#endif
