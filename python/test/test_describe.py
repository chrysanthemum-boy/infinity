# Copyright(C) 2023 InfiniFlow, Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import polars as pl

import common_values
import infinity


class TestDescribe:

    def test_describe_table(self):
        infinity_obj = infinity.connect(common_values.TEST_REMOTE_HOST)

        db = infinity_obj.get_database("default")
        db.drop_table("test_describe_table")
        db.create_table(
            "test_describe_table", {"num": "integer", "body": "varchar", "vec": "vector,5,float"}, None)
        with pl.Config(fmt_str_lengths=1000):
            res = db.describe_table("test_describe_table")
            print(res)
            # check the polars dataframe
            assert res.columns == ["column_name", "column_type", "constraint"]
