from typing import List, Tuple, Any, Optional, Dict, Literal
from pathlib import Path
from pydantic import BaseModel, Field, field_validator

class Material(BaseModel):
  albedo: Optional[Path] = None
  base_color: List[float] = [0.8, 0.8, 0.8]
  normal: Optional[Path] = None
  normal_strength: float = 1.0
  roughness: Optional[Path] = None
  roughness_value: float = 0.5
  metallic: Optional[Path] = None
  metallic_value: float = 0.0
  displacement: Optional[Path] = None
  disp_scale: float = 0.05
  ao: Optional[Path] = None
  ao_mix_factor: float = 1.0
  arm: Optional[Path] = None

  @field_validator('albedo', 'normal', 'roughness', 'metallic', 'displacement', 'ao', 'arm', mode='before')
  @classmethod
  def resolve_path(cls, v):
    if v is None or v == "":
      return None
    return Path(v).resolve()

class BlenderConfig(BaseModel):
  # Render
  samples: int
  resolution_x: int
  resolution_y: int
  use_denoising: bool
  render_engine: str = "CYCLES"
  output_blender: Path = Field(default=Path("out/scene.blend"))

  # Assets
  model_path: Path
  opening_placeholders: Optional[Path] = None
  door_asset: Optional[Path] = None
  window_asset: Optional[Path] = None
  hdri_path: Optional[Path] = None
  hdri_intensity: float = 1.0

  # Materials
  floor_material: Material
  wall_and_ceil_material: Material

  @field_validator('output_blender', mode='before')
  @classmethod
  def resolve_output_blender(cls, v):
    if v is None or v == "":
      return Path("out/scene.blend").resolve()
    return Path(v).resolve()

  @field_validator('model_path', 'opening_placeholders', 'door_asset', 'window_asset', 'hdri_path', mode='before')
  @classmethod
  def resolve_optional_path(cls, v):
    if v is None or v == "":
      return None
    return Path(v).resolve()

class OpeningInfo(BaseModel):
  center: tuple[float, float, float]  # (x, y, z)
  height: float
  rotation_z: float
  thickness: float = 0.1
  type: Literal["Door", "Window"]
  width: float

BlenderConfig.model_rebuild()
OpeningInfo.model_rebuild()