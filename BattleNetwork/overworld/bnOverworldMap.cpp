#include <algorithm>
#include <cmath>

#include "bnOverworldMap.h"


namespace Overworld {
  Map::Map(std::size_t cols, std::size_t rows, int tileWidth, int tileHeight) {
    this->tileWidth = tileWidth;
    this->tileHeight = tileHeight;
    this->cols = cols;
    this->rows = rows;
    this->tileToTilesetMap.push_back(nullptr);
  }

  void Map::Update(double elapsed) {
    for (auto& tileMeta : tileMetas) {
      if (tileMeta != nullptr) {
        tileMeta->animation.Update(elapsed, tileMeta->sprite);
      }
    }
  }

  const sf::Vector2f Map::ScreenToWorld(sf::Vector2f screen) const
  {
    auto scale = getScale();
    return OrthoToIsometric(sf::Vector2f{ screen.x / scale.x, screen.y / scale.y });
  }

  const sf::Vector2f Map::WorldToScreen(sf::Vector2f screen) const
  {
    return IsoToOrthogonal(screen);
  }

  const sf::Vector2f Map::OrthoToIsometric(const sf::Vector2f& ortho) const {
    sf::Vector2f iso{};
    iso.x = (2.0f * ortho.y + ortho.x) * 0.5f;
    iso.y = (2.0f * ortho.y - ortho.x) * 0.5f;

    return iso;
  }

  const sf::Vector2f Map::IsoToOrthogonal(const sf::Vector2f& iso) const {
    sf::Vector2f ortho{};
    ortho.x = (iso.x - iso.y);
    ortho.y = (iso.x + iso.y) * 0.5f;

    return ortho;
  }

  const sf::Vector2i Map::GetTileSize() const {
    return sf::Vector2i(tileWidth, tileHeight);
  }

  const std::string& Map::GetName() const
  {
    return name;
  }

  const std::string& Map::GetBackgroundName() const
  {
    return backgroundName;
  }

  const std::string& Map::GetSongPath() const
  {
    return songPath;
  }

  void Map::SetName(const std::string& name)
  {
    this->name = name;
  }

  void Map::SetBackgroundName(const std::string& name)
  {
    backgroundName = name;
  }

  void Map::SetSongPath(const std::string& path)
  {
    songPath = path;
  }

  const unsigned Map::GetCols() const
  {
    return cols;
  }

  const unsigned Map::GetRows() const
  {
    return rows;
  }

  unsigned int Map::GetTileCount() {
    return tileMetas.size();
  }

  std::unique_ptr<Map::TileMeta>& Map::GetTileMeta(unsigned int tileGid) {
    return tileMetas[tileGid];
  }

  std::shared_ptr<Map::Tileset> Map::GetTileset(std::string name) {
    if (tilesets.find(name) == tilesets.end()) {
      return nullptr;
    }

    return tilesets[name];
  }

  std::shared_ptr<Map::Tileset> Map::GetTileset(unsigned int tileGid) {
    if (tileToTilesetMap.size() <= tileGid) {
      return nullptr;
    }

    return tileToTilesetMap[tileGid];
  }

  void Map::SetTileset(unsigned int tileGid, unsigned int tileId, std::shared_ptr<Map::Tileset> tileset) {
    if (tileToTilesetMap.size() <= tileGid) {
      tileToTilesetMap.resize(tileGid + 1);
      tileMetas.resize(tileGid + 1);
    }

    auto tileMeta = std::make_unique<TileMeta>(
      tileId,
      tileGid,
      tileset->offset
      );

    tileMeta->sprite.setTexture(*tileset->texture);
    tileMeta->animation = tileset->animation;
    tileMeta->animation << to_string(tileId) << Animator::Mode::Loop;
    tileMeta->animation.Refresh(tileMeta->sprite);

    tileMetas[tileGid] = std::move(tileMeta);
    tileToTilesetMap[tileGid] = tileset;
    tilesets[tileset->name] = tileset;
  }

  std::size_t Map::GetLayerCount() const {
    return layers.size();
  }

  Map::Layer& Map::GetLayer(size_t index) {
    return layers[index];
  }

  Map::Layer& Map::AddLayer() {
    layers.emplace_back(Layer(cols, rows));

    return layers[layers.size() - 1];
  }

  static Map::Tile nullTile = Map::Tile(0);

  Map::Layer::Layer(std::size_t cols, std::size_t rows) {
    this->cols = cols;
    this->rows = rows;
    tiles.resize(cols * rows, nullTile);
  }

  Map::Tile& Map::Layer::GetTile(int x, int y)
  {
    if (x < 0 || y < 0 || x >= rows || y >= cols) {
      // reset nullTile as it may have been mutated
      nullTile = Map::Tile(0);
      return nullTile;
    }

    return tiles[y * rows + x];
  }

  Map::Tile& Map::Layer::GetTile(float x, float y)
  {
    return GetTile(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y)));
  }

  Map::Tile& Map::Layer::SetTile(int x, int y, Tile tile)
  {
    return tiles[y * rows + x] = tile;
  }

  Map::Tile& Map::Layer::SetTile(int x, int y, unsigned int gid)
  {
    return tiles[y * rows + x] = Map::Tile(gid);
  }

  Map::Tile& Map::Layer::SetTile(float x, float y, unsigned int gid)
  {
    return SetTile(std::floor(x), std::floor(y), gid);
  }
}


