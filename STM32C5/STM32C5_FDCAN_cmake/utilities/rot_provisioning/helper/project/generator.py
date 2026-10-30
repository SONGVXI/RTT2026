#
# ****************************************************************************
# @attention
#
# Copyright (c) 2026 STMicroelectronics.
# All rights reserved.
#
# This software is licensed under terms that can be found in the LICENSE file
# in the root directory of this software component.
# If no LICENSE file comes with this software, it is provided AS-IS.
#
# ****************************************************************************

from enum import Enum
import logging

from helper.common.log import ROT_Logger
from helper.project.project_appli import Project_Appli
from helper.project.project_generic import Project_Generic
from helper.project.project_oemirot import Project_OEMiRoT
from helper.exception.rot_exception import RoTValueError


class ProjectType(Enum):
    OEMIROT_BOOT = 1
    OEMIROT_APPLICATION = 2


def create_project(project_type: ProjectType, project_ini_path: str, rot_logger: ROT_Logger) -> Project_Generic:
    if project_type == ProjectType.OEMIROT_BOOT:
        return Project_OEMiRoT(project_ini_path, rot_logger)

    if project_type == ProjectType.OEMIROT_APPLICATION:
        return Project_Appli(project_ini_path, rot_logger)

    raise RoTValueError(
        f"Project Type '{project_type}' is invalid.")
