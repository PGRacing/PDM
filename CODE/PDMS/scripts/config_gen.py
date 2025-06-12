import json
import os
from jsonschema import validate
import sys


class PDMGenerator:
    def __init__(self, input_file, output_path):
        self.input_file = input_file
        self.output_path = output_path
        self.needs_update = self.is_update_needed(input_file)

        if self.needs_update:
            with open(input_file) as f:
                self.config = json.load(f)

            with open(
                os.path.join(os.path.dirname(input_file), "config_schema.json")
            ) as schema_file:
                schema = json.load(schema_file)
                try:
                    validate(instance=self.config, schema=schema)
                except Exception as e:
                    print(f"Error validating JSON schema: {e}")
                    sys.exit(1)

            # Ensure the output directory exists
            output_dir = os.path.dirname(os.path.join(output_path, "gen/"))
            if not os.path.exists(output_dir):
                os.makedirs(output_dir)

            output_file_name = os.path.basename(input_file).replace(".json", ".c")
            self.output_file = open(os.path.join(output_dir, output_file_name), "w")

    def is_update_needed(self, input_file) -> bool:
        update_needed = True

        with open(input_file) as f:
            current_content = f.read()
            script_dir = os.path.dirname(__file__)
            file_name = os.path.basename(input_file)
            temp_file_path = os.path.join(script_dir, "temp")
            temp_file_name = os.path.join(temp_file_path, file_name)

            if not os.path.exists(temp_file_path):
                os.makedirs(temp_file_path)

            #TODO: Uncomment the following lines after testing
            
            # if os.path.exists(temp_file_name):
            #     with open(temp_file_name, "r") as temp_file:
            #         previous_content = temp_file.read()

            #         if current_content == previous_content:
            #             print("No changes in the PDM configuration file")
            #             update_needed = False

            if update_needed:
                with open(temp_file_name, "w") as temp_file:
                    temp_file.write(current_content)

        return update_needed

    def generate_c_code(self):
        code = '#include "out.h"\n\n'
        code += "const T_OUT_CFG pdmConfig[] __attribute__((section(\".config\"))) = {\n"

        for channel in self.config.get("channels", []):
            code += f'\t[OUT_ID_{channel["id"]}] = {{\t\n'
            code += f'\t\t.id = OUT_ID_{channel["id"]},\n'
            code += f'\t\t.type = OUT_TYPE_{channel["type"]},\n'
            code += f'\t\t.mode = OUT_MODE_{channel["mode"]},\n'
            code += f'\t\t.spocId = {channel.get("spocId", "SPOC2_ID_MAX")},\n'
            code += f'\t\t.spocChId = {channel.get("spocChId","SPOC2_CH_ID_MAX")},\n'
            code += f'\t\t.name = "{channel["name"]}",\n'
            code += f'\t\t.batch = {channel.get("batch", "OUT_ID_MAX")},\n'

            safety = channel.get("safety")

            if safety is None:
                code += "\t\t.safety =  NULL,\n"
            else:
                code += "\t\t.safety = {\n"

                code += f"\t\t\t.afterErrorCfg = {{\n"
                code += f'\t\t\t\t.behavior = OUT_ERR_BEH_{safety["afterErrorCfg"]["behavior"]},\n'
                code += f'\t\t\t\t.latchTime = {safety["afterErrorCfg"].get("latchTime", "UINT32_MAX")}\n'
                code += f"\t\t\t}},\n"
                code += f'\t\t\t.actOnSafety = {str(safety["actOnSafety"]).lower()},\n'
                code += f'\t\t\t.errRetryThreshold = {safety["errRetryThreshold"]},\n'
                code += f'\t\t\t.retryTimerInterval = {safety["retryTimerInterval"]},\n'
                code += f'\t\t\t.retryCallback = {safety["retryCallback"]},\n'

                code += self.generate_soc_code(safety)
                code += self.generate_i2t_config_code(safety)
                code += "\t\t},\n"
            code += "\t},\n"

        code += "};\n\n"
        code += "const T_OUT_CFG *getPDMConfig(uint8_t *length) {\n"
        code += "\t*length = sizeof(pdmConfig) / sizeof(T_OUT_CFG);\n"
        code += "\treturn pdmConfig;\n"
        code += "}\n"

        self.output_file.write(code)
        self.output_file.close()

    def generate_soc_code(self, safety_cfg) -> str:
        soc_cfg = safety_cfg.get("socCfg")

        code = "\t\t\t.socCfg = {\n"

        if soc_cfg is None:
            code += "\t\t\t\t.useSoc = false,\n"
            code += "\t\t\t},\n"
        else:
            code += f'\t\t\t\t.useSoc = {str(soc_cfg["useSoc"]).lower()},\n'
            code += f'\t\t\t\t.nominalThreshold = {soc_cfg["nominalThreshold"]},\n'
            code += f'\t\t\t\t.allowInrush = {str(soc_cfg["allowInrush"]).lower()},\n'
            code += f'\t\t\t\t.inrushWindowFromStart = {soc_cfg["inrushWindowFromStart"]},\n'
            code += f'\t\t\t\t.inrushThreshold = {soc_cfg["inrushThreshold"]},\n'
            code += f'\t\t\t\t.inrushTimeThreshold = {soc_cfg["inrushTimeThreshold"]},\n'
            code += "\t\t\t},\n"

        return code

    def generate_i2t_config_code(self, safety_cfg) -> str:
        i2c_cfg = safety_cfg.get("i2tCfg")
        code = "\t\t\t.i2tCfg = {\n"

        if i2c_cfg is None:
            code += "\t\t\t\t.useI2t = false,\n"
            code += "\t\t\t},\n"
        else:
            code += f'\t\t\t\t.useI2t = {str(i2c_cfg["useI2t"]).lower()},\n'
            code += f'\t\t\t\t.nominalThreshold = {i2c_cfg["nominalThreshold"]},\n'
            code += f'\t\t\t\t.allowInrush = {str(i2c_cfg["allowInrush"]).lower()},\n'
            code += f'\t\t\t\t.inrushWindowFromStart = {i2c_cfg["inrushWindowFromStart"]},\n'
            code += f'\t\t\t\t.inrushThreshold = {i2c_cfg["inrushThreshold"]},\n'
            code += f'\t\t\t\t.inrushTimeThreshold = {i2c_cfg["inrushTimeThreshold"]},\n'
            code += "\t\t\t},\n"

        return code

    def generate_code(self):
        if self.needs_update:
            self.generate_c_code()


if __name__ == "__main__":
    script_dir = os.path.dirname(__file__)
    pdm_config_path = os.path.join(script_dir, "../DB/config.json")
    output_path = os.path.join(script_dir, "../Core/Src/")

    print("Generating PDM configuration code...")

    pdm_gen = PDMGenerator(pdm_config_path, output_path)
    pdm_gen.generate_code()

    print("PDM configuration code generated successfully!")
