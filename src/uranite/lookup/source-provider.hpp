//
// @author hxAri (hxari)
// @create 2026-11-21 00:00
// @update 2026-11-21 00:00
// @github https://github.com/uranite-lang/uranite
//
// Uranite Copyright (c) 2025 - hxAri <hxari@proton.me>
// Uranite Licence under GNU General Public Licence v3
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//

#ifndef _URANITE_LOOKUP_SOURCE_PROVIDER_HPP_
#define _URANITE_LOOKUP_SOURCE_PROVIDER_HPP_

#include <memory>
#include <optional>
#include <string>

namespace uranite::lookup {
	
	/**
	 * @brief Abstract provider of source text for a given path.
	 *
	 * Used by the Driver to allow in-memory overrides (e.g. unsaved editor
	 * buffers) to take precedence over the file system. A null provider (the
	 * default) means "read from disk".
	 */
	class SourceProvider {
		
		public:
			
			virtual ~SourceProvider() = default;
			
			/**
			 * @brief Reads the source text for the given path.
			 * @param path The file system path or URI to read.
			 * @return The source text if available, or std::nullopt on failure.
			 */
			virtual std::optional<std::string> read( const std::string& path ) const = 0;
		
	};
	
	using SourceProviderSharedPointer = std::shared_ptr<SourceProvider>;
	
} // namespace uranite::lookup

#endif // end _URANITE_LOOKUP_SOURCE_PROVIDER_HPP_
