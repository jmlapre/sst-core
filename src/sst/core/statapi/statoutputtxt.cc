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

#include "sst/core/statapi/statoutputtxt.h"

#include "sst/core/stringize.h"

namespace SST::Statistics {

StatisticOutputTextBase::StatisticOutputTextBase(Params& outputParameters) :
    StatisticFieldsOutput(outputParameters)
{}

bool
StatisticOutputTextBase::checkOutputParameters()
{
    const Params& params = getOutputParameters();

    m_outputTopHeader    = params.find<bool>("outputtopheader", getOutputTopHeaderDefault());
    m_outputInlineHeader = params.find<bool>("outputinlineheader", getOutputInlineHeaderDefault());
    m_outputSimTime      = params.find<bool>("outputsimtime", getOutputSimTimeDefault());
    m_outputRank         = params.find<bool>("outputrank", getOutputRankDefault());

    m_useCompression = false;

    if ( outputsToFile() ) {
        m_FilePath = params.find<std::string>("filepath", getDefaultFileName());

        // Perform some checking on the parameters
        if ( 0 == m_FilePath.length() ) {
            // Filepath is zero length
            return false;
        }

        if ( supportsCompression() ) {
            m_useCompression = params.find<bool>("compressed", false);
        }
    }

    return true;
}

void
StatisticOutputTextBase::startOfSimulation()
{
    StatisticFieldInfo*        statField;
    FieldInfoArray_t::iterator it_v;

    // Open the finalized filename
    if ( !openFile() ) return;

    // Output a Top Header if requested
    if ( true == m_outputTopHeader ) {
        // Add a Simulation Component to the front
        m_outputBuffer = "Component.Statistic";
        print("%s; ", m_outputBuffer.c_str());

        if ( true == m_outputSimTime ) {
            // Add a Simulation Time Header to the front
            m_outputBuffer = "SimTime";
            printf("%s; ", m_outputBuffer.c_str());
        }

        if ( true == m_outputRank ) {
            // Add a Rank Header to the front
            m_outputBuffer = "Rank";
            printf("%s; ", m_outputBuffer.c_str());
        }

        // Output all Headers
        it_v = getFieldInfoArray().begin();

        while ( it_v != getFieldInfoArray().end() ) {
            statField = *it_v;
            m_outputBuffer += statField->getStatName();
            m_outputBuffer += ".";
            m_outputBuffer += statField->getFieldName();

            // Increment the iterator
            it_v++;

            print("%s; ", m_outputBuffer.c_str());
        }
        print("\n");
    }
}

void
StatisticOutputTextBase::endOfSimulation()
{
    // Close the file
    closeFile();
}


void
StatisticOutputTextBase::implStartOutputEntries(StatisticBase* statistic)
{
    std::string buffer;

    // Starting Output
    // m_outputBuffer.clear();
    m_outputBuffer = getStartOutputPrefix();

    m_outputBuffer += statistic->getFullStatName();
    m_outputBuffer += " : ";
    m_outputBuffer += statistic->getStatTypeName();
    m_outputBuffer += " : ";
    if ( true == m_outputSimTime ) {
        // Add the Simulation Time to the front
        if ( true == m_outputInlineHeader ) {
            buffer = format_string("SimTime = %" PRIu64, getCurrentSimCycle());
        }
        else {
            buffer = format_string("%" PRIu64, getCurrentSimCycle());
        }

        m_outputBuffer += buffer;
        m_outputBuffer += "; ";
    }

    if ( true == m_outputRank ) {
        // Add the Rank to the front
        if ( true == m_outputInlineHeader ) {
            buffer = format_string("Rank = %d", getRank().rank);
        }
        else {
            buffer = format_string("%d", getRank().rank);
        }

        m_outputBuffer += buffer;
        m_outputBuffer += "; ";
    }
}

void
StatisticOutputTextBase::implStopOutputEntries()
{
    // Done with Output
    print("%s\n", m_outputBuffer.c_str());
}


void
StatisticOutputTextBase::outputField(fieldHandle_t fieldHandle, int32_t data)
{
    std::string         buffer;
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);

    if ( nullptr != FieldInfo ) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());

        if ( true == m_outputInlineHeader ) {
            buffer = format_string("%s.%s = %" PRId32, FieldInfo->getFieldName().c_str(), typeName, data);
        }
        else {
            buffer = format_string("%" PRId32, data);
        }
        m_outputBuffer += buffer;
        m_outputBuffer += "; ";
    }
}

void
StatisticOutputTextBase::outputField(fieldHandle_t fieldHandle, uint32_t data)
{
    std::string         buffer;
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);

    if ( nullptr != FieldInfo ) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());

        if ( true == m_outputInlineHeader ) {
            buffer = format_string("%s.%s = %" PRIu32, FieldInfo->getFieldName().c_str(), typeName, data);
        }
        else {
            buffer = format_string("%" PRIu32, data);
        }
        m_outputBuffer += buffer;
        m_outputBuffer += "; ";
    }
}

void
StatisticOutputTextBase::outputField(fieldHandle_t fieldHandle, int64_t data)
{
    std::string         buffer;
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);

    if ( nullptr != FieldInfo ) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());

        if ( true == m_outputInlineHeader ) {
            buffer = format_string("%s.%s = %" PRId64, FieldInfo->getFieldName().c_str(), typeName, data);
        }
        else {
            buffer = format_string("%" PRId64, data);
        }
        m_outputBuffer += buffer;
        m_outputBuffer += "; ";
    }
}

void
StatisticOutputTextBase::outputField(fieldHandle_t fieldHandle, uint64_t data)
{
    std::string         buffer;
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);

    if ( nullptr != FieldInfo ) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());

        if ( true == m_outputInlineHeader ) {
            buffer = format_string("%s.%s = %" PRIu64, FieldInfo->getFieldName().c_str(), typeName, data);
        }
        else {
            buffer = format_string("%" PRIu64, data);
        }
        m_outputBuffer += buffer;
        m_outputBuffer += "; ";
    }
}

void
StatisticOutputTextBase::outputField(fieldHandle_t fieldHandle, float data)
{
    std::string         buffer;
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);

    if ( nullptr != FieldInfo ) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());

        if ( true == m_outputInlineHeader ) {
            buffer = format_string("%s.%s = %f", FieldInfo->getFieldName().c_str(), typeName, data);
        }
        else {
            buffer = format_string("%f", data);
        }
        m_outputBuffer += buffer;
        m_outputBuffer += "; ";
    }
}

void
StatisticOutputTextBase::outputField(fieldHandle_t fieldHandle, double data)
{
    std::string         buffer;
    StatisticFieldInfo* FieldInfo = getRegisteredField(fieldHandle);

    if ( nullptr != FieldInfo ) {
        const char* typeName = getFieldTypeShortName(FieldInfo->getFieldType());

        if ( true == m_outputInlineHeader ) {
            buffer = format_string("%s.%s = %f", FieldInfo->getFieldName().c_str(), typeName, data);
        }
        else {
            buffer = format_string("%f", data);
        }
        m_outputBuffer += buffer;
        m_outputBuffer += "; ";
    }
}


bool
StatisticOutputTextBase::openFile()
{
    if ( !outputsToFile() ) {
        m_hFile = stdout;
        return true;
    }

    // Need to complete the filename.  For multirank jobs, add _RANK
    // to name.  Also need to get the absolute path include the set
    // output_directory
    std::string filename = m_FilePath;

    // First check for appending rank
    if ( 1 < getNumRanks().rank ) {
        int         rank    = getRank().rank;
        std::string rankstr = "_" + std::to_string(rank);

        // Search for any extension
        size_t index = filename.find_last_of(".");
        if ( std::string::npos != index ) {
            // We found a . at the end of the file, insert the rank string
            filename.insert(index, rankstr);
        }
        else {
            // No . found, append the rank string
            filename += rankstr;
        }
    }

    filename = getAbsolutePathForOutputFile(filename);

    if ( m_useCompression ) {
#ifdef HAVE_LIBZ
        m_gzFile = gzopen(filename.c_str(), "w");
        if ( nullptr == m_gzFile ) {
            // We got an error of some sort
            Output out = getSimulationOutput();
            out.fatal(CALL_INFO, 1, " : StatisticOutputCompressedTxt - Problem opening File %s - %s\n",
                m_FilePath.c_str(), strerror(errno));
            return false;
        }
#else
        return false;
#endif
    }
    else {
        m_hFile = fopen(filename.c_str(), "w");
        if ( nullptr == m_hFile ) {
            // We got an error of some sort
            Output out = getSimulationOutput();
            out.fatal(CALL_INFO, 1, " : StatisticOutputTxt - Problem opening File %s - %s\n", m_FilePath.c_str(),
                strerror(errno));
            return false;
            ;
        }
    }
    return true;
}

void
StatisticOutputTextBase::closeFile()
{
    if ( !outputsToFile() ) return;
    if ( m_useCompression ) {
#ifdef HAVE_LIBZ
        gzclose(m_gzFile);
#endif
    }
    else {
        fclose(m_hFile);
    }
}

int
StatisticOutputTextBase::print(const char* fmt, ...)
{
    int     res = 0;
    va_list args;
    if ( m_useCompression ) {
#ifdef HAVE_LIBZ
#if ZLIB_VERBUM >= 0x1271
        /* zlib added gzvprintf in 1.2.7.1.  CentOS 7 apparently uses 1.2.7.0 */
        va_start(args, fmt);
        res = gzvprintf(m_gzFile, fmt, args);
        va_end(args);
#else
        ssize_t bufSize = 128;
        bool    retry   = true;
        do {
            char* buf = (char*)malloc(bufSize);

            va_start(args, fmt);
            ssize_t n = vsnprintf(buf, bufSize, fmt, args);
            va_end(args);

            if ( n < 0 ) {
                retry = false;
            }
            else if ( n < bufSize ) {
                gzprintf(m_gzFile, "%s", buf);
                /* Success */
                retry = false;
            }
            else {
                bufSize += 128;
            }
            free(buf);
        } while ( retry );

#endif
#endif
    }
    else {
        va_start(args, fmt);
        res = vfprintf(m_hFile, fmt, args);
        va_end(args);
    }
    return res;
}

void
StatisticOutputTextBase::serialize_order(SST::Core::Serialization::serializer& ser)
{
    StatisticOutput::serialize_order(ser);
    SST_SER(m_outputTopHeader);
    SST_SER(m_outputInlineHeader);
    SST_SER(m_outputRank);
    SST_SER(m_outputSimTime);
    SST_SER(m_useCompression);
    // SST_SER(m_outputBuffer); // Rebuild during restart
    SST_SER(m_FilePath);
}

StatisticOutputTxt::StatisticOutputTxt(Params& outputParameters) :
    StatisticOutputTextBase(outputParameters)
{
    // Announce this output object's name
    Output out = getSimulationOutput();
    out.verbose(CALL_INFO, 1, 0, " : StatisticOutput%sTxt enabled...\n", m_useCompression ? "Compressed" : "");
    setStatisticOutputName(m_useCompression ? "StatisticOutputCompressedTxt" : "StatisticOutputTxt");
}

void
StatisticOutputTxt::serialize_order(SST::Core::Serialization::serializer& ser)
{
    StatisticOutputTextBase::serialize_order(ser);
}

StatisticOutputConsole::StatisticOutputConsole(Params& outputParameters) :
    StatisticOutputTextBase(outputParameters)
{
    // Announce this output object's name
    Output out = getSimulationOutput();
    out.verbose(CALL_INFO, 1, 0, " : StatisticOutputConsole enabled...\n");
    setStatisticOutputName("StatisticOutputConsole");
}

void
StatisticOutputConsole::serialize_order(SST::Core::Serialization::serializer& ser)
{
    StatisticOutputTextBase::serialize_order(ser);
}

} // namespace SST::Statistics
