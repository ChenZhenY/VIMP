function varargout = insertBackprojections(varargin)
      if length(varargin) == 5 && isa(varargin{1},'gtsam.Values') && isa(varargin{2},'gtsam.SimpleCamera') && isa(varargin{3},'double') && size(varargin{3},2)==1 && isa(varargin{4},'double') && isa(varargin{5},'double')
        gtsam_wrapper(2852, varargin{:});
      else
        error('Arguments do not match any overload of function gtsam.utilities.insertBackprojections');
      end
