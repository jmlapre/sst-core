# -*- coding: utf-8 -*-

# Copyright 2009-2025 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2025, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

""" This module is a group of global variables that must be common to all tests
"""
import os

from typing import Dict, List, TYPE_CHECKING

if TYPE_CHECKING:
    import configparser

    from test_engine_junit import JUnitTestCase

# Verbose Defines
VERBOSE_QUIET = 0
VERBOSE_NORMAL = 1
VERBOSE_LOUD = 2
VERBOSE_DEBUG = 3

# Global Var Defines
TESTRUN_TESTRUNNINGFLAG = False
TESTRUN_SINGTHREAD_TESTSUITE_NAME = ""
TESTRUN_JUNIT_TESTCASE_DICTLISTS: Dict[str, List["JUnitTestCase"]] = dict()

TESTOUTPUT_TOPDIRPATH = ""
TESTOUTPUT_RUNDIRPATH = ""
TESTOUTPUT_TMPDIRPATH = ""
TESTOUTPUT_XMLDIRPATH = ""

TESTENGINE_CONCURRENTMODE = False
TESTENGINE_THREADLIMIT = 8
TESTENGINE_DEBUGMODE = False
TESTENGINE_LOGFAILMODE = False
TESTENGINE_IGNORESKIPS = False
TESTENGINE_VERBOSITY = 1
TESTENGINE_SSTRUN_NUMRANKS = 1
TESTENGINE_SSTRUN_NUMTHREADS = 1
TESTENGINE_SSTRUN_GLOBALARGS = ""
TESTENGINE_CORE_CONFFILE_PARSER: "configparser.RawConfigParser" = None  # type: ignore [assignment]
TESTENGINE_CORE_CONFINCLUDE_DICT: Dict[str, str] = dict()
TESTENGINE_ELEM_CONFINCLUDE_DICT: Dict[str, str] = dict()
TESTENGINE_ERRORCOUNT = 0
TESTENGINE_SCENARIOSLIST: List[str] = []
TESTENGINE_TESTNOTESLIST: List[str] = []

# These are some globals to pass data between the top level test engine
# and the lower level testscripts
def init_test_engine_globals() -> None:
    """ Initialize the test global variables """
    global TESTRUN_TESTRUNNINGFLAG
    global TESTRUN_SINGTHREAD_TESTSUITE_NAME
    global TESTRUN_JUNIT_TESTCASE_DICTLISTS

    global TESTOUTPUT_TOPDIRPATH
    global TESTOUTPUT_RUNDIRPATH
    global TESTOUTPUT_TMPDIRPATH
    global TESTOUTPUT_XMLDIRPATH

    global TESTENGINE_CONCURRENTMODE
    global TESTENGINE_THREADLIMIT
    global TESTENGINE_DEBUGMODE
    global TESTENGINE_LOGFAILMODE
    global TESTENGINE_IGNORESKIPS
    global TESTENGINE_VERBOSITY
    global TESTENGINE_SSTRUN_NUMRANKS
    global TESTENGINE_SSTRUN_NUMTHREADS
    global TESTENGINE_SSTRUN_GLOBALARGS
    global TESTENGINE_CORE_CONFFILE_PARSER
    global TESTENGINE_CORE_CONFINCLUDE_DICT
    global TESTENGINE_ELEM_CONFINCLUDE_DICT
    global TESTENGINE_ERRORCOUNT
    global TESTENGINE_SCENARIOSLIST
    global TESTENGINE_TESTNOTESLIST

    TESTRUN_TESTRUNNINGFLAG = False
    TESTRUN_SINGTHREAD_TESTSUITE_NAME = ""
    TESTRUN_JUNIT_TESTCASE_DICTLISTS = {}

    TESTOUTPUT_TOPDIRPATH = os.path.abspath("./sst_test_outputs")
    TESTOUTPUT_RUNDIRPATH = os.path.abspath("{0}/run_data".format(TESTOUTPUT_TOPDIRPATH))
    TESTOUTPUT_TMPDIRPATH = os.path.abspath("{0}/tmp_data".format(TESTOUTPUT_TOPDIRPATH))
    TESTOUTPUT_XMLDIRPATH = os.path.abspath("{0}/xml_data".format(TESTOUTPUT_TOPDIRPATH))

    TESTENGINE_CONCURRENTMODE = False
    TESTENGINE_THREADLIMIT = 8
    TESTENGINE_DEBUGMODE = False
    TESTENGINE_LOGFAILMODE = False
    TESTENGINE_IGNORESKIPS = False
    TESTENGINE_VERBOSITY = 1
    TESTENGINE_SSTRUN_NUMRANKS = 1
    TESTENGINE_SSTRUN_NUMTHREADS = 1
    TESTENGINE_SSTRUN_GLOBALARGS = "xxx"
    TESTENGINE_CORE_CONFFILE_PARSER = None  # type: ignore [assignment]
    TESTENGINE_CORE_CONFINCLUDE_DICT = {}
    TESTENGINE_ELEM_CONFINCLUDE_DICT = {}
    TESTENGINE_ERRORCOUNT = 0
    TESTENGINE_SCENARIOSLIST = []
    TESTENGINE_TESTNOTESLIST = []
