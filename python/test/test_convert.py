import pytest
from infinity.errors import ErrorCode

from python.test.common import common_values
import infinity


class TestConvert:
    def test_to_pl(self):
        infinity_obj = infinity.connect(common_values.TEST_REMOTE_HOST)
        db_obj = infinity_obj.get_database("default")
        db_obj.drop_table("test_to_pl", True)
        db_obj.create_table("test_to_pl", {
            "c1": "int", "c2": "float"}, None)

        table_obj = db_obj.get_table("test_to_pl")
        table_obj.insert([{"c1": 1, "c2": 2}])
        print()
        res = table_obj.output(["c1", "c2"]).to_pl()
        print(res)
        res = table_obj.output(["c1", "c1"]).to_pl()
        print(res)
        res = table_obj.output(["c1", "c2", "c1"]).to_pl()
        print(res)

    def test_to_pa(self):
        infinity_obj = infinity.connect(common_values.TEST_REMOTE_HOST)
        db_obj = infinity_obj.get_database("default")
        db_obj.drop_table("test_to_pa", True)
        db_obj.create_table("test_to_pa", {
            "c1": "int", "c2": "float"}, None)

        table_obj = db_obj.get_table("test_to_pa")
        table_obj.insert([{"c1": 1, "c2": 2.0}])
        print()
        res = table_obj.output(["c1", "c2"]).to_arrow()
        print(res)
        res = table_obj.output(["c1", "c1"]).to_arrow()
        print(res)
        res = table_obj.output(["c1", "c2", "c1"]).to_arrow()
        print(res)

    def test_to_df(self):
        infinity_obj = infinity.connect(common_values.TEST_REMOTE_HOST)
        db_obj = infinity_obj.get_database("default")
        db_obj.drop_table("test_to_pa", True)
        db_obj.create_table("test_to_pa", {
            "c1": "int", "c2": "float"}, None)

        table_obj = db_obj.get_table("test_to_pa")
        table_obj.insert([{"c1": 1, "c2": 2.0}])
        print()
        res = table_obj.output(["c1", "c2"]).to_df()
        print(res)
        res = table_obj.output(["c1", "c1"]).to_df()
        print(res)
        res = table_obj.output(["c1", "c2", "c1"]).to_df()
        print(res)

    @pytest.mark.skip(reason="Cause core dumped.")
    def test_without_output_select_list(self):
        # connect
        infinity_obj = infinity.connect(common_values.TEST_REMOTE_HOST)
        db_obj = infinity_obj.get_database("default")
        db_obj.drop_table("test_without_output_select_list", True)
        table_obj = db_obj.create_table("test_without_output_select_list", {
            "c1": "int", "c2": "float"}, None)

        table_obj.insert([{"c1": 1, "c2": 2.0}])
        insert_res = table_obj.output([]).convert_type
        # FIXME insert_res = table_obj.output([]).to_arrow()
        # FIXME insert_res = table_obj.output([]).to_pl()
        print(insert_res)

        # disconnect
        res = infinity_obj.disconnect()
        assert res.error_code == ErrorCode.OK