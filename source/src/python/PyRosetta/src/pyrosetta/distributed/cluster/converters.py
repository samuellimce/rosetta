# :noTabs=true:
# (c) Copyright Rosetta Commons Member Institutions.
# (c) This file is part of the Rosetta software suite and is made available under license.
# (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
# (c) For more information, see http://www.rosettacommons.org. Questions about this can be
# (c) addressed to University of Washington CoMotion, email: license@uw.edu.


__author__ = "Jason C. Klima"

try:
    import git
    import toolz
except ImportError:
    print(
        "Importing 'pyrosetta.distributed.cluster.converters' requires the "
        + "third-party packages 'gitpython' and 'toolz' as dependencies!\n"
        + "Please install these packages into your python environment. "
        + "For installation instructions, visit:\n"
        + "https://gitpython.readthedocs.io/en/stable/intro.html\n"
        + "https://pypi.org/project/toolz/\n"
    )
    raise

import collections
import logging
import os
import pyrosetta
import sys
import types

from functools import singledispatch
from pyrosetta.rosetta.core.pose import Pose
from pyrosetta.distributed.cluster.converter_tasks import (
    environment_cmd,
    get_yml,
    is_bytes,
    is_dict,
    is_packed,
    parse_input_packed_pose as _parse_input_packed_pose,
    to_int,
    to_iterable,
    to_packed,
    to_str,
)
from pyrosetta.distributed.cluster.serialization import Serialization
from pyrosetta.distributed.packed_pose.core import PackedPose
from typing import (
    Any,
    Callable,
    Dict,
    Iterable,
    List,
    NoReturn,
    Optional,
    Sized,
    Tuple,
    TypeVar,
    Union,
)

S = TypeVar("S", bound=Serialization)


def _parse_decoy_ids(objs: Any) -> List[int]:
    """
    Normalize user-provided PyRosetta 'decoy_ids' to a `list` object containing `int` objects.
    """

    return to_iterable(objs, to_int, "decoy_ids")


def _parse_empty_queue(protocol_name: str, ignore_errors: bool) -> None:
    """Return a `None` object when a protocol results in an error with `ignore_errors=True`."""
    logging.warning(
        f"User-provided PyRosetta protocol '{protocol_name}' resulted in an empty queue with `ignore_errors={ignore_errors}`!"
        + "Putting a `None` object into the queue."
    )
    return None


def _parse_environment(obj: Any) -> str:
    """Parse the input `environment` attribute of PyRosettaCluster."""

    @singledispatch
    def converter(obj: Any) -> NoReturn:
        raise ValueError("'environment' must be of type `str` or `NoneType`!")

    @converter.register(type(None))
    def _parse_none(obj: None) -> str:
        yml = get_yml()
        if yml == "":
            logging.warning(
                f"`{environment_cmd}` did not run successfully, "
                + "so the active conda environment YML file string was not saved! "
                + "It is recommended to run: "
                + f"\n`{environment_cmd} > environment.yml`\n"
                + "to reproduce this simulation later."
            )
        return yml

    @converter.register(str)
    def _parse_str(obj: str) -> Union[str, NoReturn]:
        if obj == "":
            logging.warning(
                "The input 'environment' parameter argument is an empty string, "
                + "which is not a valid YML file string capturing the active conda "
                + "environment! Reproduction simulations may not necessarily reproduce "
                + "the original decoy(s)! Please verify that your active conda "
                + "environment is identical to the original conda environment that "
                + "generated the decoy(s) you wish to reproduce!"
                + "\nBypassing conda environment validation...\n"
            )
            return obj
        else:
            if obj != get_yml():
                raise RuntimeError(
                    "The 'environment' parameter argument is not equivalent to the YML file string "
                    + "generated by the active conda environment, and therefore the original "
                    + "decoy may not necessarily be reproduced. Please set the 'environment' parameter "
                    + "argument to an empty string ('') to bypass conda environment validation and run the simulation."
                )
            else:
                logging.debug(
                    "The 'environment' parameter argument correctly validated against the active conda environment!"
                )
                return obj

    return converter(obj)


def _parse_protocols(objs: Any) -> List[Union[Callable[..., Any], Iterable[Any]]]:
    """
    Parse the `protocols` argument parameters from the PyRosettaCluster().distribute() method.
    """

    @singledispatch
    def converter(objs: Any) -> NoReturn:
        raise RuntimeError(
            "The user-provided PyRosetta protocols must be an "
            + "iterable of objects of `types.GeneratorType` and/or "
            + "`types.FunctionType` types, or an instance of type "
            + "`types.GeneratorType` or `types.FunctionType`, not of type "
            + f"{type(objs)}!"
        )

    @converter.register(type(None))
    def _none_to_list(obj: None) -> List[Any]:
        return []

    @converter.register(types.FunctionType)
    @converter.register(types.GeneratorType)
    def _func_to_list(
        obj: Union[Callable[..., Any], Iterable[Any]]
    ) -> List[Union[Callable[..., Any], Iterable[Any]]]:
        return [obj]

    @converter.register(collections.abc.Iterable)
    def _to_list(
        objs: Iterable[Any],
    ) -> Union[List[Union[Callable[..., Any], Iterable[Any]]], NoReturn]:
        for obj in objs:
            if not isinstance(obj, (types.FunctionType, types.GeneratorType)):
                raise TypeError(
                    "Each member of PyRosetta protocols must be of type "
                    + "`types.FunctionType` or `types.GeneratorType`!"
                )
        return list(objs)

    return converter(objs)


def _parse_pyrosetta_build(obj: Any) -> str:
    """Parse the PyRosetta build string."""

    _pyrosetta_version_string = pyrosetta._version_string()

    @singledispatch
    def converter(obj: Any) -> NoReturn:
        raise ValueError("'pyrosetta_build' must be of type `str` or `NoneType`!")

    @converter.register(type(None))
    def _default_none(obj: None) -> str:
        return _pyrosetta_version_string

    @converter.register(str)
    def _validate_pyrosetta_version_string(obj: str) -> str:
        if obj != _pyrosetta_version_string:
            logging.warning(
                f"The user-provided PyRosetta build string '{obj}' is not equal "
                + f"to the currently used PyRosetta build string '{_pyrosetta_version_string}'! "
                + "Therefore, the original decoy may not necessarily be reproduced! "
                + "Using different PyRosetta builds can lead to different outputs. "
                + "Please consider running this simulation using the PyRosetta build that "
                + "was used with the original simulation run."
            )
        return _pyrosetta_version_string

    return converter(obj)


def _parse_scratch_dir(obj: Any) -> str:
    """Parse the input `scratch_dir` attribute of PyRosettaCluster."""

    @singledispatch
    def converter(obj: Any) -> NoReturn:
        raise ValueError(
            f"'scratch_dir' directory {obj} could not be created or found!"
        )

    @converter.register(str)
    def _is_str(obj: str) -> str:
        return obj

    @converter.register(type(None))
    def _default_none(obj: None) -> str:
        temp_path = os.sep + "temp"
        if os.path.exists(temp_path):
            scratch_dir = temp_path
        else:
            scratch_dir = os.getcwd()
        return scratch_dir

    return converter(obj)


def _parse_seeds(objs: Any) -> List[str]:
    """
    Normalize user-provided PyRosetta 'seeds' to a `list` object containing `str` objects.
    """

    return to_iterable(objs, to_str, "seeds")


def _parse_sha1(obj: Any) -> str:
    """Parse the input `sha1` attribute of PyRosettaCluster."""

    @singledispatch
    def converter(obj: Any) -> NoReturn:
        raise ValueError(f"'sha1' attribute must be of type `str` or `NoneType`!")

    @converter.register(str)
    def _register_str(obj: str) -> Union[str, NoReturn]:
        """Parse `str` type inputs to the 'sha1' attribute."""

        try:
            repo = git.Repo(".", search_parent_directories=True)
        except git.InvalidGitRepositoryError as ex:
            raise git.InvalidGitRepositoryError(
                f"{ex}\nThe script being executed is not in a git repository! "
                + "It is strongly recommended to use version control for your "
                + "scripts. To continue without using version control, set the "
                + "`sha1` attribute of PyRosettaCluster to `None`."
            )
        # Path to and name of the git repository
        git_root = repo.git.rev_parse("--show-toplevel")
        repo_name = os.path.split(git_root)[-1]
        try:
            commit = repo.head.commit
        except ValueError as ex:
            raise git.InvalidGitRepositoryError(
                f"{ex}\nNo HEAD commit present! Is this a repository with no commits?"
            )
        if obj == "":
            # Ensure that everything in the work directory is included in the repository
            if len(repo.untracked_files):
                raise git.InvalidGitRepositoryError(
                    "There are untracked files in your git repository! "
                    + "If these are important for the simulation, you will not "
                    + "be able to reproduce your work! Commit the changes if "
                    + "appropriate, or add untracked files to your .gitignore file "
                    + "before running PyRosettaCluster!"
                )
            # Ensure that all changes to tracked files are committed
            if repo.is_dirty(untracked_files=True):
                raise git.InvalidGitRepositoryError(
                    "The working directory is dirty! "
                    + "Commit local changes to ensure reproducibility."
                )
            return commit.hexsha
        else:
            # A sha1 was provided. Validate that it is HEAD
            if not commit.hexsha.startswith(obj):
                logging.error(
                    "A `sha1` attribute was provided to PyRosettaCluster, "
                    + "but it appears that you have not checked out this revision!"
                    + f"Please run on the command line:\n`git checkout {obj}`\n"
                    + "and then re-execute this script."
                )
                raise RuntimeError(
                    "The `sha1` attribute provided to PyRosettaCluster "
                    + "does not match the current `HEAD`! "
                    + "See log files for details on resolving the issue."
                )
            return obj

    @converter.register(type(None))
    def _default_none(obj: None) -> str:
        """If the user provides `None` to the `sha1` attribute, return an empty string."""

        return ""

    return converter(obj)


def _parse_system_info(obj: Any) -> Dict[Any, Any]:
    """Parse the input `system_info` attribute of PyRosettaCluster."""

    _sys_platform = {"sys.platform": sys.platform}

    @singledispatch
    def converter(obj: Any) -> NoReturn:
        raise ValueError("'system_info' must be of type `dict` or `NoneType`!")

    @converter.register(type(None))
    def _default_none(obj: None) -> Dict[str, str]:
        return _sys_platform

    @converter.register(dict)
    def _overwrite_sys_platform(obj: Dict[Any, Any]) -> Dict[Any, Any]:
        if "sys.platform" in obj:
            _obj_sys_platform = obj["sys.platform"]
            _curr_sys_platform = _sys_platform["sys.platform"]
            if _obj_sys_platform != _curr_sys_platform:
                logging.warning(
                    "The dictionary key 'sys.platform' of the PyRosettaCluster "
                    + "'system_info' attribute indicates a previously used system "
                    + f"platform '{_obj_sys_platform}', but the currently used "
                    + f"system platform is '{_curr_sys_platform}'! Therefore, the original "
                    + "decoy may not necessarily be reproduced! Platform-specific "
                    + "information can lead to different outputs. Please consider "
                    + "running this simulation in a Docker container with the same "
                    + "system platform as the original simulation run, or switching "
                    + "to the original system platform before reproducing this simulation."
                )
        return toolz.dicttoolz.merge(obj, _sys_platform)

    return converter(obj)


def _get_decoy_id(protocols: Sized, decoy_ids: List[int]) -> Optional[int]:
    """Get the decoy number given the user-provided PyRosetta protocols."""

    if decoy_ids:
        decoy_id_index = (len(decoy_ids) - len(protocols)) - 1
        decoy_id = decoy_ids[decoy_id_index]
    else:
        decoy_id = None

    return decoy_id


def _get_packed_poses_output_kwargs(
    result: Any,
    input_kwargs: Dict[Any, Any],
    protocol_name: str,
) -> Tuple[List[PackedPose], Dict[Any, Any]]:
    packed_poses = []
    protocol_kwargs = []
    for obj in to_iterable(result, to_packed, protocol_name):
        if is_packed(obj):
            packed_poses.append(obj)
        elif is_dict(obj):
            protocol_kwargs.append(obj)

    if len(packed_poses) == 0:
        packed_poses = to_iterable(None, to_packed, protocol_name)

    if len(protocol_kwargs) == 0:
        output_kwargs = input_kwargs
    elif len(protocol_kwargs) == 1:
        output_kwargs = next(iter(protocol_kwargs))
        output_kwargs.update(
            toolz.dicttoolz.keyfilter(lambda k: k.startswith("PyRosettaCluster_"), input_kwargs)
        )
    elif len(protocol_kwargs) >= 2:
        raise ValueError(
            f"User-provided PyRosetta protocol '{protocol_name}' may return at most one object of type `dict`."
        )

    return packed_poses, output_kwargs


def _get_compressed_packed_pose_kwargs_pairs_list(
    packed_poses: List[PackedPose],
    output_kwargs: Dict[Any, Any],
    protocol_name: str,
    protocols_key: str,
    decoy_ids: List[int],
    serializer: S,
) -> List[Tuple[bytes, bytes]]:
    decoy_id = _get_decoy_id(output_kwargs[protocols_key], decoy_ids)
    compressed_packed_pose_kwargs_pairs_list = []
    for i, packed_pose in enumerate(packed_poses):
        if (decoy_id != None) and (i != decoy_id):
            logging.info(
                "Discarding a returned decoy because it does not match the user-provided 'decoy_ids'."
            )
            continue
        task_kwargs = serializer.deepcopy_kwargs(output_kwargs)
        if "PyRosettaCluster_decoy_ids" not in task_kwargs:
            task_kwargs["PyRosettaCluster_decoy_ids"] = []
        task_kwargs["PyRosettaCluster_decoy_ids"].append((protocol_name, i))
        compressed_packed_pose = serializer.compress_packed_pose(packed_pose)
        compressed_task_kwargs = serializer.compress_kwargs(task_kwargs)
        compressed_packed_pose_kwargs_pairs_list.append((compressed_packed_pose, compressed_task_kwargs))

    if decoy_id:
        assert (
            len(compressed_packed_pose_kwargs_pairs_list) == 1
        ), "When specifying decoy_ids, there may only be one decoy_id per protocol."

    return compressed_packed_pose_kwargs_pairs_list


def _parse_protocol_results(
    result: Any,
    input_kwargs: Dict[Any, Any],
    protocol_name: str,
    protocols_key: str,
    decoy_ids: List[int],
    serializer: S,
) -> List[Tuple[bytes, bytes]]:
    """Parse results from the user-provided PyRosetta protocol."""
    packed_poses, output_kwargs = _get_packed_poses_output_kwargs(result, input_kwargs, protocol_name)
    compressed_packed_pose_kwargs_pairs_list = _get_compressed_packed_pose_kwargs_pairs_list(
        packed_poses, output_kwargs, protocol_name, protocols_key, decoy_ids, serializer
    )

    return compressed_packed_pose_kwargs_pairs_list


def _parse_target_results(objs: List[Tuple[bytes, bytes]]) -> List[Tuple[bytes, bytes]]:
    """Parse results returned from the spawned thread."""

    ids = set()
    n_obj = 0
    for obj in objs:
        if len(obj) != 2:
            raise TypeError("Returned result should be of length 2.")
        n_obj += len(obj)
        ids.update(set(map(id, obj)))
    assert len(ids) == n_obj, "Returned results do not have unique memory addresses."

    return objs


def _parse_tasks(objs):
    """Parse the input `tasks` attribute of PyRosettaCluster."""

    @singledispatch
    def converter(objs: Any) -> NoReturn:
        raise ValueError(
            "Parameter passed to 'tasks' argument must be an iterable, a function, a generator, or None!"
        )

    @converter.register(dict)
    def _from_dict(obj: Dict[Any, Any]) -> List[Dict[Any, Any]]:
        return [obj]

    @converter.register(type(None))
    def _from_none(obj: None) -> List[Dict[Any, Any]]:
        logging.warning(
            "PyRosettaCluster `tasks` attribute was set to `None`! Using a default empty task."
        )
        return [{}]

    @converter.register(types.FunctionType)
    def _from_function(
        obj: Callable[..., Iterable[Any]]
    ) -> Union[List[Dict[Any, Any]], NoReturn]:
        _tasks = []
        for obj in objs():
            if isinstance(obj, dict):
                _tasks.append(obj)
            else:
                raise TypeError(
                    f"Each task must be of type `dict`. Received '{obj}' of type `{type(obj)}`."
                )
        return _tasks

    @converter.register(collections.abc.Iterable)
    @converter.register(types.GeneratorType)
    def _from_iterable(obj: Iterable[Any]) -> Union[List[Dict[Any, Any]], NoReturn]:
        _tasks = []
        for obj in objs:
            if isinstance(obj, dict):
                _tasks.append(obj)
            else:
                raise TypeError(
                    f"Each task must be of type `dict`. Received '{obj}' of type `{type(obj)}`."
                )
        return _tasks

    return converter(objs)
